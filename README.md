# TriforceLinuxSyscallFuzzer
* 20160613
* https://github.com/nccgroup/TriforceLinuxSyscallFuzzer
* Jess Hertz <jesse.hertz@nccgroup.trust>
* Tim Newsham <tim.newsham@nccgroup.trust>

This is a collection of files used to perform system call
fuzzing of Linux x86_64 kernels using AFL and QEMU.  To use 
it you will need TriforceAFL from https://github.com/nccgroup/TriforceAFL
and a kernel image to fuzz.  Scripts assume that TriforceAFL is
found in `$TAFL` or `../TriforceAFL/` (N.B. building `testAfl` requires
that `../TriforceAFL/config.h` exist).

## Building
To build:
```
  make
```

## Fuzzing
To run, first install a kernel into `./kern/bzImage` and 
extract `/proc/kallsyms`
into `./kern/kallsyms`. Set `K=kern` environment variable to point to
your kernel. Now run:

```
  make inputs
  ./runFuzz -M M0
```

Note that the `runFuzz` script expects a master or slave name, as
it always runs in master/slave mode.  See the `runFuzz` script for
more usage information.

Also Note that this only creates a small set of example inputs.  To test
a large number of important system calls, you will probably want to
generate one example of each system call, or at least one example
for every "shape" of system call.  These should be placed in `inputs/`.
See `gen2.py` for an example.

## Reproducing
To reproduce test cases (such as crashes) run:

```
  ./runTest inputs/ex1
  ./runTest outputs/crashes/id*
```

You can also run the driver out of the emulated environment
with the `-t` option, with verbose logging with `-vv`
and without actually performing the system calls with `-x`:

```
  ./driver -tvvx < inputs/ex1
  strace ./driver -t < inputs/ex1
```

It is sometimes useful to be able to boot the kernel and interactively 
run tests.  To do so, edit the rootTemplate files as you see fit (for
example, to add more test tools to the root filesystem), then run:

```
  ./runCmd
```

Other commands other than the shell can be invoked by specifying
them as command line arguments to `runCmd`.
Note: when done with the shell, use ```^A-c``` to get the QEMU prompt
and type ```quit```.

## Debugging
Debugging is easiest with a kernel built with debugging symbols enabled.
Use `runTest` to start the kernel and run a test through the
driver, or use `runCmd` to manually run a test case from the shell.
Edit your run script to include the `-s` option when starting `afl-qemu-system-trace`.
This will enable `gdb` support on TCP port 1234.  Use `getvmlinux` to extract
the `vmlinux` kernel image from your `bzImage` kernel and run gdb after
the system has booted:
```
   cp kern/bzImage .
   ./getvmlinux
   gdb ./vmlinux
   target remote :1234
   break somefunction
   continue
```
You can attach the debugger after `runTest` has caused a crash
or before you manually trigger then bug in `runCmd`.

Note that Linux sources are compiled with optimization turned
on by default. This can make debugging confusing and difficult.
You can disable optimization on a file-by-file
basis by editing the Linux make file for the subdirectory a file is
in and adding `CFLAGS_name.o = -O0` to the `Makefile`.  For
example editing `kernel/Makefile` and adding `CFLAGS_sys_ni.o = -O0`
will disable optimization when building `kernel/sys_ni.o`.


# Bugs

Note: When fuzzing a Linux 2.* kernel you will need to enable
the CPU timer.  When the timer is not enabled panic and logging
detection do not seem to operate properly and panics result
in hangs.  To enable the timer, call `startForkserver(1)` in
`driver.c` instead of `startForkserver(0)`.  This issue
does not seem to occur in Linux3.* and Linux4.* kernels.

