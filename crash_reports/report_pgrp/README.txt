Init's children can crash Linux 2 kernels.

Risk: Low

Description:
A process that is in the same process group as the ``init'' process 
(group id zero) can crash the Linux 2 kernel with several system calls
by passing in a process ID or process group ID of zero.  The
value zero is a special value that indicates the current process
ID or process group. However, in this case it is also the process
group ID of the process.

Of these calls, the ``getpriority'', ``setpriority'' and 
``iorpriorityget'' calls can be executed as any user.  The other 
two calls, ``ioprioset'' and ``kill'' only cause crashes when 
executed as root.

Reproduction:
Compile the following attached files.  Boot a linux kernel with
a shell script /bin/init which spawns /bin/sh.  Execute the test
programs from this shell.

  - repro-pgrp-getpriority.c - runs with euid=1000
  - repro-pgrp-setpriority.c - runs with euid=1000
  - repro-pgrp-ioprioget.c   - runs with euid=1000

  - repro-pgrp-ioprioset.c   - runs as root
  - repro-pgrp-kill.c        - runs as root


NCC was able to reproduce this issues on the following kernel
built for the x86_64 target:

  - linux-2.6.32.71 - defconfig

NCC was unable to reproduce this issue on the following kernels:

  - linux-3.18.29   - defconfig
  - linux-4.5.0       - defconfig


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