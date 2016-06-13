/* 
 * Syscall driver for heating the JIT cache.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>

#include "drv.h"
#include "sysc.h"

static void usage(char *prog) {
    printf("usage:  %s [-vx] files..\n", prog);
    printf("\t\t-v\tverbose mode\n");
    printf("\t\t-x\tdon't perform system call\n");
    exit(1);
}

int verbose = 0;
int noSyscall = 0;

void
heat(char *fn)
{
    char buf[1024];
    struct sysRec recs[3];
    struct slice slice;
    int pid, st, x, fd, nrecs, sz;

    if((pid = fork()) == 0) {
        printf("heating %s ...\n", fn);
        fd = open(fn, O_RDONLY);
        if(fd == -1) {
            perror(fn);
            exit(1);
        }
        sz = read(fd, buf, sizeof buf);
        close(fd);
        if(sz < 0) {
            perror(fn);
            exit(1);
        }
        //printf("got work: %d - %.*s\n", sz, (int)sz, buf);

        mkSlice(&slice, buf, sz);
        x = parseSysRecArr(&slice, 3, recs, &nrecs);
        if(verbose) {
            printf("read %d bytes, parse result %d nrecs %d\n", sz, x, (int)nrecs);
            if(x == 0)
                showSysRecArr(recs, nrecs);
        }
        if(!noSyscall) {
            x = doSysRecArr(recs, nrecs);
            if (verbose) printf("syscall returned %d\n", x);
        }
        exit(0);

    } else {
        waitpid(pid, &st, WNOHANG);
    }
}

int
main(int argc, char **argv)
{
    char *prog;
    int i, opt;

    prog = argv[0];
    while((opt = getopt(argc, argv, "tvx")) != -1) {
        switch(opt) {
        case 'v':
            verbose++;
            break;
        case 'x':
            noSyscall = 1;
            break;
        case '?':
        default:
            usage(prog);
            break;
        }
    }
    argc -= optind;
    argv += optind;
    if(!argc) {
        printf("no files!\n");
        exit(1);
    }

    for(i = 0; argv[i]; i++) {
        heat(argv[i]);
    }
    return 0;
}

