Root user can panic Linux 2 and Linux 3 kernels by unmounting root.

Risk: Low

Description:
If the root user calls the umount2 system call to unmount root ("/")
with the flags 2 (MNT_FORCE) or 3 (MNT_FORCE | MNT_DETACH)
Linux 2 and Linux 3 kernels will panic.  Linux 4 kernels do not
seem affected.

Reproduction:
Compile the attached repro-umount2.c file and execute it as root.

NCC was able to reproduce this issues on the following kernels
built for the x86_64 target:

  - linux-2.6.32.71 - defconfig
  - linux-3.18.29   - defconfig

The linux-2.6.32.71 kernel stops with: "kernel BUG at fs/pnode.c:330!".
The linux-3.18.29 kernel stops with "kernel BUG at fs/pnode.c:372!".

NCC was unable to reproduce this issue on the following kernel:

  - linux-4.5       - defconfig


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