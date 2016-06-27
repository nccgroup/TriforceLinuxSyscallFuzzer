/* crashes linux2.* with a NULL pointer issue
 * Works when getpgrp() == 0.
 *
 * Note: sometimes this just causes a hang, sometimes it causes a BUG:
 *
 * BUG: unable to handle kernel NULL pointer dereference at 00000000
 * [<ffffffff810501fb>] __send_signal+0x1ba/0x1e1
 * [<ffffffff8105027f>] send_signal+0x5d/0x68
 * [<ffffffff81050968>] do_send_sig_info+0x3e/0x6f
 * [<ffffffff81050b15>] group_send_sig_info+0x31/0x39
 * [<ffffffff81050b5c>] __kill_pgrp_info+0x3f/0x6c
 * [<ffffffff81051add>] sys_kill+0xd5/0x164
 */

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

int main(int argc, char **argv)
{
    kill(0, 9);
    return 0;
}
