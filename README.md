# TriforceLinuxSyscallFuzzer
* 20160613
* https://github.com/nccgroup/TriforceLinuxSyscallFuzzer
* Jess Hertz <jesse.hertz@nccgroup.trust>
* Tim Newsham <tim.newsham@nccgroup.trust>

This is a collection of files used to perform system call
fuzzing of Linux x86_64 kernels using AFL and QEMU.  To use 
it you will need TriforceAFL from https://github.com/nccgroup/TriforceAFL
and a kernel image to fuzz.  Scripts assume that TriforceAFL is
found in `$TAFL` or `../TriforceAFL/`.

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
  ./runCmd sh -i
```

Note: when done with the shell, use ```^A-c``` to get the QEMU prompt
and type ```quit```.

