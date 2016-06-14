# Internals

# Driver

The driver is responsible for receiving test inputs from AFL (or
stdin), parsing them into a number of system call records, and then executing
each system call.  It is implemented in `driver.c`.  It has command-line
options for running in test-mode (without AFL), for operating more
verbosely and for skipping the system call step.

In normal operation the driver first forks off a child that becomes
the main process and then waits for the child to die.  This is useful
to catch cases where the driver dies before indicating to AFL that work is
complete.

Next the driver starts the AFL fork server.  Everything after this
point happens in a forked copy of the emulator.  It then calls
`getWork` to get some work from AFL.
Next it calls `startWork` to start tracing the driver while
it parses the input data.
Then it calls `startWork` again to sotp tracing the driver and
start tracing the kernel.
Finally it performs the system calls indicated by the parsed
call records before calling `doneWork` to notify AFL that the
test case is complete.

When run from AFL, everything between the `startForkserver` and
`doneWork` call will be executed in a forked copy of the virtual
machine.  This process will be repeated once for each input file.
If the driver crashes before reachign the `doneWork` call, the
watcher will catch it and call `doneWork` on behalf of the crashed
child.   

Note that in unusual situations the fuzzer may perform some of these
calls out of order, confusing QEMU and causing it to crash the
forked copy of the virtual machine.  This can happen when a `clone`
call jumps back to code that calls `startForkserver`, `getWork`, or
`startWork`, for example.

When run in test mode, the start and stop calls are skipped and
input is read from `stdin` instead of from AFL.

# makeRoot

`makeRoot` is a shell script that copies the `rootTemplate` directory,
copies in the driver, and then patches `init` to run the driver.
It is used by the run scripts to build an appropriate root filesystem
for the test in question.  The script contains some documentation
at the top of the file.

# Run scripts
The `runFuzz`, `runTest` and `runCmd` scripts provide convenient
ways to start fuzzing or perform reproduction steps.  All three
scripts require a kernel to be present in `$K/bzImage` (or `kern/bzImage`
if `$K` is not set).  To properly detect panic and log messages,
the `/proc/kallsyms` file from the kernel should also be present
in `$K/kallsyms` (or `kern/kallsyms`).  These scripts contain
some documentation at the top of each script file.

* `runFuzz` starts up AFL fuzzing of files in `inputs` and writes outputs
to `outputs`.  It can restart an existing fuzzer by using `-C`.
It requires `-M name` or `-S name` arguments to specify a master or
slave name.
* `runTest` runs a set of input files sequentially through the driver.
The inputs are specified on the command line.
* `runCmd` boots a kernel and runs a command.  If no arguments are
specified it will execute a shell otherwise it runs the specified
command, which should exist in `rootTemplate/bin`.

# AFL Test Driver
The `testAfl.c` program is used by `runTest` to run test cases
through the driver program.  It uses the same protocol that AFL
uses to talk to QEMU to start and run the fork server as it feeds
each input to the driver.

