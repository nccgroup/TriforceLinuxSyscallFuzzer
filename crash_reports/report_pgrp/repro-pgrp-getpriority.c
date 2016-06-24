/* crashes linux2.* with a NULL pointer issue
 * Works when getpgrp() == 0.
 */

#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

int main(int argc, char **argv)
{
    setuid(1000); /* even works as nobody! */
    getpriority(1, 0);
    return 0;
}
