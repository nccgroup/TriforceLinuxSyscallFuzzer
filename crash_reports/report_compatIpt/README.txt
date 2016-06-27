Corrupted offset allows for arbitrary decrements in compat IPT_SO_SET_REPLACE setsockopt

Risk: High

Description:
When processing an IPT_SO_SET_REPLACE setsockopt request made with
the compat_setsockopt system call (which requires CONFIG_COMPAT=y
and CONFIG_IP_NF_IPTABLES=m or CONFIG_IP_NF_IPTABLES=y), 
the kernel will alter arbitrary kernel memory through pointers provided
by the caller (if CONFIG_MODULE_UNLOAD=y is configured). This
can be leveraged to elevate privileges or to gain arbitrary code 
execution in the kernel.  This call requires root permissions, but can
be invoked by an arbitrary user if CONFIG_USER_NS=y and
CONFIG_NET_NS=y are enabled in the kernel.

Due to incomplete validation of target_offset values in 
check_compat_entry_size_and_hooks() in net/ipv4/netfilter/ip_tables.c,
a critical offset can be corrupted. As a result, several important 
structures are referenced from unvalidated memory during error cleanup.
These structures are meant to contain kernel-provided data,
but a malicious user can provide these values. The result
is that a malicious user can decrement arbitrary kernel integers
when they are positive.

In check_compat_entry_size_and_hooks() the entry is validated with

    ret = check_entry((struct ipt_entry *)e, name); 

This checks that target_offset is not too big, but does not
check if it is too small!

If target_offset is small,
check_compat_entry_size_and_hooks() will not iterate over
ematch's and initialize them with:

    xt_ematch_foreach(ematch, e) {
        ret = compat_find_calc_match(ematch, name, &e->ip, &off);
        if (ret != 0)
            goto release_matches;
        ++j;
    }

but a small target_offset (such as 74) can cause the target
pointer to alias parts of the "e" entry, because "t"
is calculated as "e + e->target_offset" here:

    t = compat_ipt_get_target(e);

Later this value is written to:

    target = xt_request_find_target(NFPROTO_IPV4, t->u.user.name,
                    t->u.user.revision);
    if (IS_ERR(target)) {
        duprintf("check_compat_entry_size_and_hooks: `%s' not found\n",
             t->u.user.name);
        ret = PTR_ERR(target);
        goto release_matches;
    }
    t->u.kernel.target = target;

The write to t->u.kernel.target can corrupt the e->target_offset
if the target aliases part of the entry.

Later when iterating over the same object in compat_release_entry()
it can iterate over matches that didn't exist earlier (when target_offset
was too small to contain any). These matches were never properly
initialized:

    /* Cleanup all matches */
    xt_ematch_foreach(ematch, e)
        module_put(ematch->u.kernel.match->me);

This results in an uninitialized pointer for ematch->u.kernel.match.
This pointer ematch->u.kernel.match->me now comes from user
data instead of trusted kernel data!

Using a target_offset of 74 causes target_offset to be overwritten
with the high two bytes of "target" which will always be 0xffff.

When module_put is called, it uses the value of ematch->u.kernel.match->me
and decrements it's refcnt field (at offset 824 in the 4.5 kernel when
using the default configuration) with atomic_dec_if_positive(). An 
attacker can provide a malicious value for the "me" pointer to 
decrement any positive 32-bit integer in kernel memory space.

After the ematch's module has been decremented, the entry's own
module is also decremented:

    t = compat_ipt_get_target(e);
    module_put(t->u.kernel.target->me);

This also uses the corrupted target pointer, now at offset 65535
from the entry, whose kernel values were never initialized. providing 
another opportunity for decrementing an arbitrary pointer.

Note that the behavior of module_put varies a bit from kernel
to kernel.  In Linux-4.5 it calls 
"atomic_dec_if_positive(&module->refcnt)" to
decrement an attacker provided pointer.  This allows any memory
in the kernels virtual address space to be decremented provided
it is positive.  By decrementing overlapping unaligned memory,
an attacker can craft arbitrary values at the expense of corrupting
adjacent memory.

The Linux-2.6.32.71's version of module_put looks up the cpu
number and calls "local_dec(__module_ref_addr(module, cpu))".
This results in a decrement of the value pointed to by
"module->refptr + __per_cpu_offset[cpu]".  If an attacker knows
the per_cpu_offset they can craft a decrement to any address
in the kernels virtual memory.  The value is decremented
whether or not it is positive.

The Linux-3.18.25 kernel's version of module_put() calls 
"__this_cpu_inc(module->refptr->decs)".  On x86_64 this
results in an "incq   %gs:0x8(%rax)" instruction. This
allows an attacker to perform arbitrary increments but
only to memory referenced through the "%gs" segment.

This same issue is likely exploitable through other entry
points since the code for the netfilters for IP, IPv6,
ARP and bridge are closely related and share common code:

  net/ipv4/netfilter/arp_tables.c
  net/ipv4/netfilter/ip_tables.c
  net/ipv6/netfilter/ip6_tables.c
  net/bridge/netfilter/ebtables.c

It also appears that the same issue may exist in the non-compatible
code case.  The find_check_entry() also iterates over a set
of matches, and if an error is detected, iterates it once again
to cleanup the matches.  It also assigns "t->u.kernel.target"
which can alias the entry record and its matches.  Triggering
this condition would involve more careful crafting of entry
records since the normal translate_table() function does a lot
more validation before calling find_check_entry().


Reproduction:
Compile the following binary as a 32-bit binary (ie. with -m32) and run 
it as the root user or in a new USER_NS and NET_NS as container root:

   - repro-compatReleaseEntry.c

This will cause a NULL pointer dereference while dereferencing
bytes passed in by the attack program.  This has been confirmed to
cause a panic or BUG message on the following kernels built for x86_64:

   - Linux-2.6.32.71 defconfig
   - Linux 3.18.25 defconfig
   - Linux-4.5.0 defconfig

The defect is in portable code and should work on other platforms
whenever there is support for 32-bit binaries to run on a 64-bit
Linux kernel.


The attached uns.c program can be used to run this in a
user namespace with the following arguments (assuming
that the user is uid=1000 and that CONFIG_USER_NS=y
and CONFIG_NET_NS=y are enabled for the kernel):

   uns -n -U -M '0 1000 1' -G '0 1000 1' ./repro-compatReleaseEntry
   
We are also providing an example of how to abuse this to modify
arbitrary memory.  It works only for Linux-4.*.
It will decrement an integer in the programs memory space,
but the decrement happens in the kernel through an arbitrary pointer
provided by the program, and could reference kernel memory instead.
Compile the following binary as a 32-bit binary (ie. with -m32) and run 
it as the root user or in a new USER_NS and NET_NS as container root:

   - repro-compatReleaseEntryMod.c

NCC has confirmed that this test case runs properly on Linux-4.5.0
with the default configuration compiled for x86_64.


Recommendation:
Verify that e->target_offset is at least as large as sizeof(*e) in
check_entry() in net/ipv4/netiflter/ip_tables.c.
This should prevent the structures referenced through target_offset
from aliasing the entry itself. The function already checks
that target_offset does not reference memory belonging to any
following entries. Reproduce these validation fixes in the
other netfilter table implementations.

Implement similar validation checks in 
check_entry_size_and_hooks() to guard against the possibility
of similar issues in the normal (non-compatibility) version
of the setsockopt call.  Reproduce these fixes in the other
netfilter table implementations.

This issue arose because structures copied from user memory
were augmented with kernel-trusted data.  These structures
contain a union where information is first read from the user-provided
data and then used to populate kernel-trusted data. These
practices are dangerous. Simple errors in book-keeping can allow
user-provided data to be misinterpretted as trusted kernel data.
We recommend that these practices be discontinued in
the long term to make it less likely that user data could be
confused for trusted kernel data by an attacker.  A safer solution
would be to allocate a kernel structure to contain the kernel-trusted
data followed by user-provided data, and to copy the user-provided
data into the appropriate parts of this structure.

