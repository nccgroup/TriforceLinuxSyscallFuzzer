Out of bounds reads when processing IPT_SO_SET_REPLACE setsockopt 

Risk: Medium

Description:
When installing an IP filter with the setsockopt and
the IPT_SO_SET_REPLACE command, the input record (struct ipt_replace)
and its payload (struct ipt_entry records) are not properly
validated. The entry's target_offset fields are not validated
to be in bounds and can reference kernel memory outside of
the user-provided data.  This results in out-of-bounds reads 
being performed on kernel data adjacent to the copied user data.
It may also allow out-of-bounds writes to adjacent data. These
issues can result in kernel BUG messages and information disclosure
and possibly heap corruption.  The target_offset field is 16-bits 
and can only reference a limited amount of data past the end of 
the user-provided data.  This issue is present when 
CONFIG_IP_NF_IPTABLES=m or CONFIG_IP_NF_IPTABLES=y has been configured.

The IPT_SO_SET_REPLACE command triggers a call to translate_table(),
in net/ipv4/netfilter/ip_tables.c, which is responsible for copying 
and translating the replace request's table of entries into kernel 
structures.  It iterates over the list of entries calling 
check_entry_size_and_hooks() for each entry. This call validates the 
entry but does not validate the entry's target_offset field, which 
references the target as an offset from the entry record.  
check_entry_size_and_hooks() will also iterate over any valid hooks 
and will call check_underflow() on the entry if it is an underflow hook.  
This function accesses the target via the unvalidated target_offset and 
reads the target's u.user.name and verdict fields.  These reads
can be out of bounds, and can access adjacent heap data or
can lead to a page fault and a kernel BUG report.  If either
of these fields does not pass a validation check, the 
check_entry_size_and_hooks() will print a log message to the
dmesg buffer reporting the validation failure. This happens
when the u.user.name field does not have the empty string or
the verdict field does not have the value -1 or -2.  The
presence or absence of the logging message can be used to
infer information about adjacent heap data.

After returning, translate_table() accesses the target's 
u.user.name field using the target_offset.
This access can be out of bounds and can result in a kernel BUG.

After translate_table() iterates over the entries it performs
further validation steps which can also access targets through
the target_offset. It then iterates over the entries again
calling find_check_entry() for each entry.  This function can
perform a write to a kernel-internal field of the target,
which can corrupt adjacent heap data. A malicious attacker
attempting to abuse this issue would not have much control
over the value that is written to the target memory.
NCC has not determined if this out of bounds write can be 
triggered or if the earlier validation steps prevent it from
being reachable.

This same issue is likely exploitable through other entry
points since the code for the netfilters for IP, IPv6,
ARP and bridge are closely related and share common code:
  net/ipv4/netfilter/arp_tables.c
  net/ipv4/netfilter/ip_tables.c
  net/ipv6/netfilter/ip6_tables.c
  net/bridge/netfilter/ebtables.c

As an aside, the kernel will allocate and copy in large amounts
of user data based on a 32-bit size provided by the caller.
This size is limited only by this check in xt_alloc_table_info():

    /* Pedantry: prevent them from hitting BUG() in vmalloc.c --RR */
    if ((SMP_ALIGN(size) >> PAGE_SHIFT) + 2 > totalram_pages)
        return NULL;

An attacker may be able to consume large amounts of kernel memory
with multiple simultaneous calls.


Reproduction:
Compile and run the following attached file as the root user
or in a new USER_NS and NET_NS as container root:

  - repro-translateTable.c 

It will trigger a memory fault in do_replace() when trying
to strcmp the user's name from iter + iter->target_offset.

The attached uns.c program can be used to run this in a
user namespace with the following arguments (assuming
that the user is uid=1000 and that CONFIG_USER_NS=y
and CONFIG_NET_NS=y are enabled for the kernel):

   uns -n -U -M '0 1000 1' -G '0 1000 1' ./repro-translateTable

NCC was able to reproduce this issues on the following kernels
built for the x86_64 target:

  - linux-2.6.32.71 - defconfig
  - linux-3.18.29   - defconfig
  - linux-4.5.0       - defconfig


Recommendation:
Check the validity of e->target_offset in check_entry_size_and_hooks()
in net/ipv4/netfilter/ip_tables.c.  Ensure that the resulting
target pointer at target_offset is fully contained within the entry's
size and does not overlap the entry record itself. Reproduce
these validation fixes in the other netfilter table implementations.


#####

About NCC:
NCC Group is a security consulting company that performs all manner of
security testing and has a strong desire to help make the industry a
better, more resilient place. Because of this, when NCC Group
identifies vulnerabilities in a system they prefer to work closely with
vendors to create more secure systems. NCC Group strongly believes in
responsible disclosure, and has strict guidelines in place to ensure
that proper disclosure procedure is followed at all times. This serves
the dual purpose of allowing the vendor to safely secure the product or
system in question as well as allowing NCC Group to share cutting edge
research or advisories with the security community.
