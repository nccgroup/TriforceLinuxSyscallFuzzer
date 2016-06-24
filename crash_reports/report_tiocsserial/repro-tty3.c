
#include <sys/ioctl.h>
#include <linux/serial.h>

int main(int argc, char **argv)
{
    struct serial_struct buf;
    ioctl(0, TIOCGSERIAL, &buf);
    buf.flags = 0xf800;
    buf.custom_divisor = 0x021f;
    buf.baud_base = 0x10000000; /* requires root to change this */
    ioctl(0, TIOCSSERIAL, &buf);
    return 0;
}

