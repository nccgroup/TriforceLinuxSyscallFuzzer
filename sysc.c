#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/stat.h>

#include "drv.h"
#include "sysc.h"

extern int verbose;

/* internal syscall arg parsing state */
#define NSLICES 7
#define STKSZ 256
struct parseState {
    struct sysRec *calls;
    int ncalls;
    struct slice slices[NSLICES];
    u_int64_t sizeStk[STKSZ];
    size_t nslices, bufpos, stkpos;
};

static int parseArg(struct slice *b, struct parseState *st, u_int64_t *x);

static int pushSize(struct parseState *st, u_int64_t sz)
{
    if(st->stkpos >= STKSZ)
        return -1;
    size_t stkpos = st->stkpos++;
    st->sizeStk[stkpos] = sz;
    return 0;
}

static int popSize(struct parseState *st, u_int64_t *sz)
{
    if(st->stkpos == 0)
        return -1;
    size_t stkpos = --st->stkpos;
    *sz = st->sizeStk[stkpos];
    return 0;
}

static void dumpContents(unsigned char *buf, size_t sz)
{
    size_t i;

    if(verbose > 1) {
        printf("contents: ");
        for(i = 0; i < sz; i++)
            printf("%02x", buf[i]);
        printf("\n");
    }
}

static int parseArgNum(struct slice *b, struct parseState *st, u_int64_t *x)
{
    if(getU64(b, x) == -1)
        return -1;
    if(verbose) printf("argNum %llx\n", (unsigned long long)*x);
    return 0;
}

/* pass a buffer as an argument */
static int parseArgAlloc(struct slice *b, struct parseState *st, u_int64_t *x)
{
    void *p;
    u_int32_t sz;

    if(getU32(b, &sz) == -1)
        return -1;
    p = malloc(sz); /* note: we ignore memory leaks - exit/doneWork are perfect GCs */
    if(!p
    || pushSize(st, sz) == -1)
        return -1;
    memset(p, 0, sz);
    *x = (u_int64_t)(u_long)p;
    if(verbose) printf("argAlloc %llx - allocated %x bytes\n", (unsigned long long)*x, sz);
    return 0;
}

/* pass a buffer as an argument */
static int parseArgBuf(struct slice *b, struct parseState *st, u_int64_t *x)
{
    if(st->bufpos >= st->nslices)
        return -1;
    size_t pos = st->bufpos++;
    struct slice *bslice = st->slices + pos;
    size_t sz = sliceSize(bslice);
    if(pushSize(st, sz) == -1)
        return -1;
    *x = (u_int64_t)(u_long)sliceBuf(bslice);
    if(verbose) printf("argBuf %llx from %ld bytes\n", (unsigned long long)*x, sz);
    dumpContents(sliceBuf(bslice), sz);
    return 0;
}

/* pass a buffer length as an argument */
static int parseArgBuflen(struct slice *b, struct parseState *st, u_int64_t *x)
{
    if(popSize(st, x) == -1)
        return -1;
    if(verbose) printf("argBuflen %llx\n", (unsigned long long)*x);
    return 0;
}

/* make a file with buffer contents and set arg to fd open to the start of that file */
static int parseArgFile(struct slice *b, struct parseState *st, u_int64_t *x)
{
    static int num = 0;
    char namebuf[128];
    int fd;

    if(st->bufpos >= st->nslices)
        return -1;
    size_t pos = st->bufpos++;
    struct slice *bslice = st->slices + pos;

    snprintf(namebuf, sizeof namebuf - 1, "/tmp/file%d", num++);
    fd = open(namebuf, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(fd == -1
    || write(fd, sliceBuf(bslice), sliceSize(bslice)) == -1
    || lseek(fd, 0, SEEK_SET) == -1) {
        perror(namebuf);
        exit(1);
    }
    fchmod(fd, 0777); // just in case it previously existed with other mode
    *x = fd;
    if(verbose) printf("argFile %llx - %ld bytes from %s\n", (unsigned long long)*x, (u_long)sliceSize(bslice), namebuf);
    dumpContents(sliceBuf(bslice), sliceSize(bslice));
    return 0;
}

static int parseArgStdFile(struct slice *b, struct parseState *st, u_int64_t *x)
{
    int fd;
    unsigned short typ;

    if(getU16(b, &typ) == -1)
        return -1;
    fd = getStdFile(typ);
    if(fd == -1)
        return -1;
    *x = fd;
    if(verbose) printf("argStdFile %llx - type %d\n", (unsigned long long)*x, typ);
    return 0;
}

static int parseArgVec64(struct slice *b, struct parseState *st, u_int64_t *x)
{
    u_int64_t *vec;
    int i;
    u_int8_t sz;

    if(getU8(b, &sz) == -1)
        return -1;
    vec = malloc(sz * sizeof vec[0]); /* note: we ignore memory leaks - exit/doneWork are perfect GCs */
    if(sz && !vec)
        return -1;
    if(verbose) printf("argVec64 %llx - size %d\n", (unsigned long long)(u_long)vec, sz);
    for(i = 0; i < sz; i++) {
        if(verbose) printf("vec %d: ", i);
        if(parseArg(b, st, &vec[i]) == -1)
            return -1;
    }
    if(pushSize(st, sz) == -1)
        return -1;
    *x = (u_int64_t)(u_long)vec;
    return 0;
}

/* make a file with buffer contents and set arg to point to its filename */
static int parseArgFilename(struct slice *b, struct parseState *st, u_int64_t *x)
{
    static int num = 0;
    char namebuf[128];
    int fd;

    if(st->bufpos >= st->nslices)
        return -1;
    size_t pos = st->bufpos++;
    struct slice *bslice = st->slices + pos;

    snprintf(namebuf, sizeof namebuf - 1, "/tmp/file%d", num++);
    fd = open(namebuf, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if(fd == -1
    || write(fd, sliceBuf(bslice), sliceSize(bslice)) == -1
    || close(fd) == -1) {
        perror(namebuf);
        exit(1);
    }
    *x = (u_int64_t)(u_long)strdup(namebuf); /* note: we ignore memory leaks - exit/doneWork are perfect GCs */
    if(verbose) printf("argFilename %llx - %ld bytes from %s\n", (unsigned long long)*x, (u_long)sliceSize(bslice), namebuf);
    dumpContents(sliceBuf(bslice), sliceSize(bslice));
    return 0;
}

static int
mkChild(u_int64_t *retPid)
{
    pid_t pid;
    int i;

    fflush(stdout);
    pid = fork();
    switch(pid) {
    case -1: return -1;
    case 0:
        break;
    default:
        *retPid = pid;
        return 0;
    }

    /* child process */
    for(i = 0; i < 3; i++)
        sleep(1);
    exit(0);
}

/* use a pid related to our process as an arg */
static int parseArgPid(struct slice *b, struct parseState *st, u_int64_t *x)
{
    unsigned char typ;

    if(getU8(b, &typ) == -1)
        return -1;
    switch(typ) {
    case 0: // my pid
        *x = getpid(); 
        break;
    case 1: // parent pid
        *x = getppid(); 
        break;
    case 2: // child pid
        if(mkChild(x) == -1)
            return -1;
        break;
    default:
        return -1;
    }
    if(verbose) printf("argPid %llx - %d\n", (unsigned long long)*x, typ);
    return 0;
}

/* Reference a previously defined argument as a new arg */
static int parseArgRef(struct slice *b, struct parseState *st, u_int64_t *x)
{
    unsigned char ncall, narg;

    if(getU8(b, &ncall) == -1
    || getU8(b, &narg) == -1
    || ncall >= st->ncalls
    || narg >= 6)
        return -1;
    *x = st->calls[ncall].args[narg];
    if(verbose) printf("argRef %llx - %d %d\n", (unsigned long long)*x, ncall, narg);
    return 0;
}

static int parseArgVec32(struct slice *b, struct parseState *st, u_int64_t *x)
{
    u_int64_t elem;
    u_int32_t *vec;
    int i;
    u_int8_t sz;

    if(getU8(b, &sz) == -1)
        return -1;
    vec = malloc(sz * sizeof vec[0]); /* note: we ignore memory leaks - exit/doneWork are perfect GCs */
    if(sz && !vec)
        return -1;
    if(verbose) printf("argVec32 %llx - size %d\n", (unsigned long long)(u_long)vec, sz);
    for(i = 0; i < sz; i++) {
        if(verbose) printf("vec %d: ", i);
        if(parseArg(b, st, &elem) == -1)
            return -1;
        vec[i] = elem;
    }
    if(pushSize(st, sz) == -1)
        return -1;
    *x = (u_int64_t)(u_long)vec;
    return 0;
}

static int parseArg(struct slice *b, struct parseState *st, u_int64_t *x)
{
    unsigned char typ;

    if(getU8(b, &typ) == -1)
        return -1;
    switch(typ) {
    case 0: return parseArgNum(b, st, x);
    case 1: return parseArgAlloc(b, st, x);
    case 2: return parseArgBuf(b, st, x);
    case 3: return parseArgBuflen(b, st, x);
    case 4: return parseArgFile(b, st, x);
    case 5: return parseArgStdFile(b, st, x);
    case 7: return parseArgVec64(b, st, x);
    case 8: return parseArgFilename(b, st, x);
    case 9: return parseArgPid(b, st, x);
    case 10: return parseArgRef(b, st, x);
    case 11: return parseArgVec32(b, st, x);
    default: return -1;
    }
}

int parseSysRec(struct sysRec *calls, int ncalls, struct slice *b, struct sysRec *x)
{
    struct parseState st;
    int i;

    /* chop input into several slices */
    if(getDelimSlices(b, BUFDELIM, sizeof BUFDELIM-1, NSLICES, st.slices, &st.nslices) == -1
    || st.nslices < 1)
        return -1;

    b = &st.slices[0];
    st.bufpos = 1;
    st.stkpos = 0;
    st.calls = calls;
    st.ncalls = ncalls;
    if(getU16(b, &x->nr) == -1)
        return -1;
    if(verbose) printf("call %d\n", x->nr);
    for(i = 0; i < 6; i++) {
        if(verbose) printf("arg %d: ", i);
        if(parseArg(b, &st, &x->args[i]) == -1)
            return -1;
    }
    return 0;
}

int parseSysRecArr(struct slice *b, int maxRecs, struct sysRec *x, int *nRecs)
{
    struct slice slices[10];
    size_t i, nslices;

    if(maxRecs > 10)
        maxRecs = 10;
    if(getDelimSlices(b, CALLDELIM, sizeof CALLDELIM-1, maxRecs, slices, &nslices) == -1)
        return -1;

    for(i = 0; i < nslices; i++) {
        if(parseSysRec(x, i, slices + i, x + i) == -1)
            return -1;
    }
    *nRecs = nslices;
    return 0;
}

void
showSysRec(struct sysRec *x)
{
    printf("syscall %d (%lx, %lx, %lx, %lx, %lx, %lx)\n", x->nr, (u_long)x->args[0], (u_long)x->args[1], (u_long)x->args[2], (u_long)x->args[3], (u_long)x->args[4], (u_long)x->args[5]);
}

void
showSysRecArr(struct sysRec *x, int n)
{
    int i;

    for(i = 0; i < n; i++)
        showSysRec(x + i);
}

unsigned long
doSysRec(struct sysRec *x)
{
    /* XXX consider doing this in asm so we can use the real syscall entry instead of the syscall() function entry */
    return syscall(x->nr, x->args[0], x->args[1], x->args[2], x->args[3], x->args[4], x->args[5]);
}

unsigned long
doSysRecArr(struct sysRec *x, int n)
{
    unsigned long ret;
    int i;

    ret = 0;
    for(i = 0; i < n; i++)
        ret = doSysRec(x + i);
    return ret;
}
