/* crashes linux2.* with a NULL pointer issue
 * Works when getpgrp() == 0.
 */

#include <unistd.h>
#include <sys/syscall.h>

int main(int argc, char **argv)
{
    /* this one requires root */
    syscall(SYS_ioprio_set, 2, 0, 0x614a);
    return 0;
}
