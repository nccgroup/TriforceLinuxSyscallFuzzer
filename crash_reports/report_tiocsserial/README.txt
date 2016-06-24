Root can cause a crash on x86_64 through serial port.

Risk: Low

Description:
Making TIOCSSERIAL ioctl calls on the serial device (tested with
an 8250 device) can cause NULL-pointer accesses, WARN_ON
messages and division-by-zero errors.  Although some of the
ioctl features are accessible to non-root users, the features
that lead to crash appear only to be accessible to root (in
the initial user namespace).

Reproduction:
Compile the attached files and execute them as root with the 
serial port device on stdin.  This can be done by booting in a 
serial console and executing the programs from a shell.

  - repro-tty1.c - Causes a NULL pointer access in mem_serial_in()
  - repro-tty2.c - Causes a NULL pointer access in mem32_serial_in()
  - repro-tty3.c - Causes a division-by-zero in do_con_write()

NCC was able to reproduce these issues on the following kernels
built for the x86_64 target using a 8250 serial device as console:

  - linux-2.6.32.71 - defconfig
  - linux-3.18.29   - defconfig
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