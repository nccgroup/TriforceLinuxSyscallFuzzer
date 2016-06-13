# TriforceLinuxSyscallFuzzer
* 20160613
* https://github.com/nccgroup/TriforceLinuxSyscallFuzzer
* Jess Hertz <jesse.hertz@nccgroup.trust>
* Tim Newsham <tim.newsham@nccgroup.trust>

This is a collection of files used to perform system call
fuzzing of Linux x86_64 kernels using AFL and QEMU.  To use 
it you will need TriforceAFL from https://github.com/nccgroup/TriforceAFL
and a kernel image to fuzz.

First install a kernel into ./kern/bzImage and extract /proc/kallsyms
into ./kern/kallsyms.  Now run:

```
  make
  make inputs
  export KERN=kern
  ./runFuzz
```

To reproduce crashes run:

```
  export KERN=kern
  ./runTest outputs/crashes/id*
```

It is sometimes useful to be able to boot the kernel and interactively 
run tests.  To do so, edit the rootTemplate files as you see fit, then run:

```
  export KERN=kern
  ./runShell
```

