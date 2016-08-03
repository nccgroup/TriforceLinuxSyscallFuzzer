/*
 * Run an instrumented binary with test inputs, playing the part of afl-fuzzer.
 * This is useful for debugging the forkserver and instrumented code and
 * for reproducing test cases.
 *
 * gcc -g -Wall testAfl.c -o testAfl
 * ./testAfl ./instrprog args with @@ in them
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "../TriforceAFL/config.h"

#define FUZZFN ".fuzzdat"

static int forceQuit = 0;

void xperror(int cond, char *msg) {
    if(cond) {
        perror(msg);
        exit(1);
    }
}

void writeFile(char *fn, char *buf) {
    FILE *fp = fopen(fn, "w");
    xperror(!fp, fn);
    fwrite(buf, 1, strlen(buf), fp);
    fclose(fp);
}

void copyFile(char *dst, char *src) {
    char ch;
    FILE *o = fopen(dst, "w");
    FILE *i = fopen(src, "r");

    xperror(!o, dst);
    xperror(!i, src);
    while(fread(&ch, 1, 1, i) == 1)
        fwrite(&ch, 1, 1, o);
    fclose(i);
    fclose(o);
}

int srv[2];
unsigned char *map;

int workpid = -1;

static void
alarmHandler(int sig)
{
    if(workpid != -1) {
        kill(workpid, SIGKILL);
        printf("\ntimeout\n");
    }
}

static void
intHandler(int sig)
{
    forceQuit = 1;
    signal(SIGINT, SIG_DFL);
}

void
runTest(char *fname, char *dat)
{
    int status, cnt, i, x;

    if(dat)
        writeFile(FUZZFN, dat);
    else
        copyFile(FUZZFN, fname);

    memset(map, 0, MAP_SIZE);
    x = write(srv[1], "GOGO", 4);
    xperror(x != 4, "write go");

    x = read(srv[0], &workpid, 4);
    xperror(x != 4, "read pid");
    printf("test running in pid %d\n", workpid);
    fflush(stdout);
    alarm(2);
    do {
        x = read(srv[0], &status, 4);
    } while(x == -1 && errno == EINTR);
    xperror(x != 4, "read status");
    alarm(0);
    workpid = -1;
    printf("test ended with status %x\n", status);
    cnt = 0;
    for(i = 0; i < MAP_SIZE; i++) {
        if(map[i]) cnt++;
    }
    printf("%d edges\n\n", cnt);
}

static double timeDelta(struct timeval *start, struct timeval *end)
{
    struct timeval d;

    timersub(end, start, &d);
    return d.tv_sec + d.tv_usec / 1000000.0;
}

int main(int argc, char **argv)
{
    char **files;
    char idbuf[20], buf[100];
    struct timeval startBoot, startTest, now;
    int p[2], id, x, pid, status, i;

    signal(SIGALRM, alarmHandler);
    signal(SIGINT, intHandler);
    xperror(argc < 2, "bad usage");

    /* make forkserver pipes */
    x = pipe(p);
    xperror(x == -1, "pipe1");
    dup2(p[0], FORKSRV_FD);
    srv[1] = p[1];
    x = pipe(p);
    xperror(x == -1, "pipe2");
    dup2(p[1], FORKSRV_FD+1);
    srv[0] = p[0];
    
    id = shmget(IPC_PRIVATE, MAP_SIZE, IPC_CREAT | IPC_EXCL | 0600);
    map = shmat(id, NULL, 0);
    xperror(id == -1 || map == NULL, "shmget");
    sprintf(idbuf, "%d", id);

    //setenv("AFLGETWORK", "1", 1);
    setenv("__AFL_SHM_ID", idbuf, 1);
    //setenv("AFL_INST_RATIO", "50", 1);

    argv += 1;
    for(i = 1; argv[i]; i++) {
        if(strcmp(argv[i], "--") == 0)
            break;
        if(strcmp(argv[i], "@@") == 0) 
            argv[i] = FUZZFN;
    }
    if(argv[i]) {
        argv[i] = 0;
        i++;
    }
    files = argv + i;

    /* run program to start forkserver */
    gettimeofday(&startBoot, 0);
    pid = fork();
    if(pid == 0) {
        close(srv[0]);
        close(srv[1]);
        execvp(argv[0], argv);
        xperror(1, argv[0]);
    }

    x = read(srv[0], buf, 4);
    xperror(x != 4, "wait forkserver");

    gettimeofday(&startTest, 0);
    for(i = 0; files[i] && !forceQuit; i++) {
        gettimeofday(&now, NULL);
        printf("Input from %s at time %ld.%06ld\n", files[i], (u_long)now.tv_sec, (u_long)now.tv_usec);
        runTest(files[i], 0);
    }
    if(i == 0)
        printf("No files to test!\n");

    close(srv[0]);
    close(srv[1]);
    waitpid(pid, &status, 0);
    printf("fork server ended with status %x\n", status);

    gettimeofday(&now, 0);
    printf("boot time:  %.2f\n", timeDelta(&startBoot, &startTest));
    printf("test time:  %.2f\n", timeDelta(&startTest, &now));
    printf("total time: %.2f\n", timeDelta(&startBoot, &now));
    if(i != 0) {
        printf("tests:     %d\n", i);
        printf("execs/sec: %.2f\n", i / timeDelta(&startTest, &now));
    }

    shmctl(id, IPC_RMID, NULL);
    return 0;
}


