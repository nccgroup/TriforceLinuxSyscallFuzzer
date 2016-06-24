/* 
 * Syscall driver
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "drv.h"
#include "sysc.h"

static void usage(char *prog) {
    printf("usage:  %s [-tvx]\n", prog);
    printf("\t\t-t\ttest mode, dont use AFL hypercalls\n");
    printf("\t\t-T\tenable qemu's timer in forked children\n");
    printf("\t\t-v\tverbose mode\n");
    printf("\t\t-x\tdon't perform system call\n");
    exit(1);
}

/* 
 * catch the driver if it dies and end the test successfully.
 * The driver getting killed is good behavior, not a kernel flaw.
 */
static void watcher(void) {
    int pid, status;

    if((pid = fork()) == 0)
        return;

    waitpid(pid, &status, 0);
    /* if we got here the driver died */
    doneWork(0);
    exit(0);
}

int verbose = 0;

int
main(int argc, char **argv)
{
    struct sysRec recs[3];
    struct slice slice;
    char *prog, *buf;
    u_long sz;
    int x, opt, nrecs;
    int noSyscall = 0;
    int enableTimer = 0;

    prog = argv[0];
    while((opt = getopt(argc, argv, "tTvx")) != -1) {
        switch(opt) {
        case 't':
            aflTestMode = 1;
            break;
        case 'T':
            enableTimer = 1;
            break;
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
    if(argc)
        usage(prog);

    if(!aflTestMode)
        watcher();
    startForkserver(enableTimer);
    buf = getWork(&sz);
    //printf("got work: %d - %.*s\n", sz, (int)sz, buf);

    /* trace our driver code while parsing workbuf */
    extern void _start(), __libc_start_main();
    startWork((u_long)_start, (u_long)__libc_start_main);
    mkSlice(&slice, buf, sz);
    x = parseSysRecArr(&slice, 3, recs, &nrecs);
    if(verbose) {
        printf("read %ld bytes, parse result %d nrecs %d\n", sz, x, (int)nrecs);
        if(x == 0)
            showSysRecArr(recs, nrecs);
    }

    /* trace kernel code while performing syscalls */
    startWork(0xffffffff81000000L, 0xffffffffffffffffL);
    if(noSyscall) {
        x = 0;
    } else if(x == 0) {
        /* note: if this crashes, watcher will do doneWork for us */
        x = doSysRecArr(recs, nrecs);
    } else {
        x = getpid(); // dummy system call
    }
    if (verbose) printf("syscall returned %d\n", x);
    doneWork(0);
    return 0;
}

