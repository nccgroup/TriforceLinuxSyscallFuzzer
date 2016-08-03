/* 
 * Syscall driver
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>

#include "drv.h"
#include "sysc.h"

#define MAXFILTCALLS 10

static void usage(char *prog) {
    printf("usage:  %s [-tvx] [-f nr]*\n", prog);
    printf("\t\t-f nr\tFilter out cases that dont make this call. Can be repeated\n");
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

static int
parseU16(char *p, unsigned short *x)
{
    unsigned long val;
    char *endp;

    val = strtoul(p, &endp, 10);
    if(endp == p || *endp != 0
    || val < 0 || val >= 65536)
        return -1;
    *x = val;
    return 0;
}

/* return true if we should execute this call */
static int
filterCalls(unsigned short *filtCalls, int nFiltCalls, struct sysRec *recs, int nrecs) 
{
    int i, j, match;

    /* all records should have calls on the filtCalls list */
    for(i = 0; i < nrecs; i++) {
        match = 0;
        for(j = 0; j < nFiltCalls; j++) {
            if(recs[i].nr == filtCalls[j])
                match = 1;
        }
        /* note: empty list is a match */
        if(!match && nFiltCalls > 0) 
            return 0;
    }
    return 1;
}

int verbose = 0;

int
main(int argc, char **argv)
{
    struct sysRec recs[3];
    struct slice slice;
    unsigned short filtCalls[MAXFILTCALLS];
    char *prog, *buf;
    u_long sz;
    long x;
    int opt, nrecs, nFiltCalls, parseOk;
    int noSyscall = 0;
    int enableTimer = 0;

    nFiltCalls = 0;
    prog = argv[0];
    while((opt = getopt(argc, argv, "f:tTvx")) != -1) {
        switch(opt) {
        case 'f': 
            if(nFiltCalls >= MAXFILTCALLS) {
                printf("too many -f args!\n");
                exit(1);
            }
            if(parseU16(optarg, &filtCalls[nFiltCalls]) == -1) {
                printf("bad arg to -f: %s\n", optarg);
                exit(1);
            }
            nFiltCalls++;
            break;
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
    parseOk = parseSysRecArr(&slice, 3, recs, &nrecs);
    if(verbose) {
        printf("read %ld bytes, parse result %d nrecs %d\n", sz, parseOk, (int)nrecs);
        if(parseOk == 0)
            showSysRecArr(recs, nrecs);
    }

    if(parseOk == 0 && filterCalls(filtCalls, nFiltCalls, recs, nrecs)) {
        /* trace kernel code while performing syscalls */
        startWork(0xffffffff81000000L, 0xffffffffffffffffL);
        if(noSyscall) {
            x = 0;
        } else {
            /* note: if this crashes, watcher will do doneWork for us */
            x = doSysRecArr(recs, nrecs);
        }
        if (verbose) printf("syscall returned %ld\n", x);
    } else {
        if (verbose) printf("Rejected by filter\n");
    }
    fflush(stdout);
    doneWork(0);
    return 0;
}

