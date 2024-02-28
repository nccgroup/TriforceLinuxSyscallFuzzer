#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>
#include <netinet/in.h>
//#include <linux/memfd.h>
//#include <sys/syscall.h>

#include "drv.h"
#include "sysc.h"

/* to ease compilation when memfd isnt supported on compilation host */
#define MFD_CLOEXEC     0x0001U
#define MFD_ALLOW_SEALING   0x0002U
#define SYS_memfd_create 319 /* x86 */
static int memfd_create(const char *name, unsigned int flags) {
    return syscall(SYS_memfd_create, name, flags);
}

int getStdFile(int typ)
{
    int fd, pipes[2];

    fd = -1;
    switch(typ) {
#define F(n, fn, flg) case n: fd = open(fn, flg); break;
    F(0, "/", O_RDONLY);
    F(1, "/proc/uptime", O_RDONLY);
#define S(n, a,b,c) case n: fd = socket(a, b, c); break;
    S(2, AF_INET, SOCK_STREAM, 0);
    S(3, AF_INET, SOCK_DGRAM, 0);
    S(4, AF_UNIX, SOCK_STREAM, 0);
    S(5, AF_UNIX, SOCK_DGRAM, 0);

    case 6: 
        if(pipe(pipes) == -1) return -1;
        fd = pipes[0];
        break;
    case 7:
        if(pipe(pipes) == -1) return -1;
        fd = pipes[1];
        break;

#define EV(n, i, fl) case n: fd = eventfd(i, fl); break;
    EV( 8, 0, 0);
    EV( 9, 1, EFD_CLOEXEC);
    EV(10, 2, EFD_NONBLOCK);
    EV(11, 3, EFD_SEMAPHORE);
    EV(12, 4, EFD_CLOEXEC | EFD_NONBLOCK)
    EV(13, 5, EFD_CLOEXEC | EFD_SEMAPHORE)
    EV(14, 6, EFD_NONBLOCK | EFD_SEMAPHORE)
    EV(15, 7, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE)

#define EP(n, x) case n: fd = epoll_create(x); break;
    EP(16, 1);
    EP(17, EPOLL_CLOEXEC);

#define IN(n, x) case n: fd = inotify_init1(x); break;
    IN(18, 0);
    IN(19, IN_NONBLOCK);
    IN(20, IN_CLOEXEC);
    IN(21, IN_NONBLOCK | IN_CLOEXEC);

#define MEM(n, nm, fl) case n: fd = memfd_create(nm, fl); break;
    MEM(22, "memfd1", 0);
    MEM(23, "memfd2", MFD_CLOEXEC);
    MEM(24, "memfd3", MFD_ALLOW_SEALING);
    MEM(25, "memfd4", MFD_CLOEXEC | MFD_ALLOW_SEALING);

#define TIM(n, t, fl) case n: fd = timerfd_create(t, fl); break;
    TIM(26, CLOCK_REALTIME, 0);
    TIM(27, CLOCK_REALTIME, TFD_NONBLOCK);
    TIM(28, CLOCK_REALTIME, TFD_CLOEXEC);
    TIM(29, CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
    TIM(30, CLOCK_MONOTONIC, 0);
    TIM(31, CLOCK_MONOTONIC, TFD_NONBLOCK);
    TIM(32, CLOCK_MONOTONIC, TFD_CLOEXEC);
    TIM(33, CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    S(34, AF_UNIX, SOCK_STREAM, 0);
    S(35, AF_UNIX, SOCK_DGRAM, 0);
    S(36, AF_UNIX, SOCK_SEQPACKET, 0);
    S(37, AF_UNIX, SOCK_RAW, 0);
    S(38, AF_UNIX, SOCK_RDM, 0);
    S(39, AF_UNIX, SOCK_PACKET, 0);
    S(40, AF_INET, SOCK_STREAM, 0);
    S(41, AF_INET, SOCK_DGRAM, 0);
    S(42, AF_INET, SOCK_SEQPACKET, 0);
    S(43, AF_INET, SOCK_RAW, IPPROTO_RAW);
    S(44, AF_INET, SOCK_RDM, 0);
    S(45, AF_INET, SOCK_PACKET, 0);
    S(46, AF_INET6, SOCK_STREAM, 0);
    S(47, AF_INET6, SOCK_DGRAM, 0);
    S(48, AF_INET6, SOCK_SEQPACKET, 0);
    S(49, AF_INET6, SOCK_RAW, IPPROTO_RAW);
    S(50, AF_INET6, SOCK_RDM, 0);
    S(51, AF_INET6, SOCK_PACKET, 0);
    S(52, AF_IPX, SOCK_STREAM, 0);
    S(53, AF_IPX, SOCK_DGRAM, 0);
    S(54, AF_IPX, SOCK_SEQPACKET, 0);
    S(55, AF_IPX, SOCK_RAW, 0);
    S(56, AF_IPX, SOCK_RDM, 0);
    S(57, AF_IPX, SOCK_PACKET, 0);
    S(58, AF_NETLINK, SOCK_STREAM, 0);
    S(59, AF_NETLINK, SOCK_DGRAM, 0);
    S(60, AF_NETLINK, SOCK_SEQPACKET, 0);
    S(61, AF_NETLINK, SOCK_RAW, 0);
    S(62, AF_NETLINK, SOCK_RDM, 0);
    S(63, AF_NETLINK, SOCK_PACKET, 0);
    S(64, AF_X25, SOCK_STREAM, 0);
    S(65, AF_X25, SOCK_DGRAM, 0);
    S(66, AF_X25, SOCK_SEQPACKET, 0);
    S(67, AF_X25, SOCK_RAW, 0);
    S(68, AF_X25, SOCK_RDM, 0);
    S(69, AF_X25, SOCK_PACKET, 0);
    S(70, AF_AX25, SOCK_STREAM, 0);
    S(71, AF_AX25, SOCK_DGRAM, 0);
    S(72, AF_AX25, SOCK_SEQPACKET, 0);
    S(73, AF_AX25, SOCK_RAW, 0);
    S(74, AF_AX25, SOCK_RDM, 0);
    S(75, AF_AX25, SOCK_PACKET, 0);
    S(76, AF_ATMPVC, SOCK_STREAM, 0);
    S(77, AF_ATMPVC, SOCK_DGRAM, 0);
    S(78, AF_ATMPVC, SOCK_SEQPACKET, 0);
    S(79, AF_ATMPVC, SOCK_RAW, 0);
    S(80, AF_ATMPVC, SOCK_RDM, 0);
    S(81, AF_ATMPVC, SOCK_PACKET, 0);
    S(82, AF_APPLETALK, SOCK_STREAM, 0);
    S(83, AF_APPLETALK, SOCK_DGRAM, 0);
    S(84, AF_APPLETALK, SOCK_SEQPACKET, 0);
    S(85, AF_APPLETALK, SOCK_RAW, 0);
    S(86, AF_APPLETALK, SOCK_RDM, 0);
    S(87, AF_APPLETALK, SOCK_PACKET, 0);
    S(88, AF_PACKET, SOCK_STREAM, 0);
    S(89, AF_PACKET, SOCK_DGRAM, 0);
    S(90, AF_PACKET, SOCK_SEQPACKET, 0);
    S(91, AF_PACKET, SOCK_RAW, 0);
    S(92, AF_PACKET, SOCK_RDM, 0);
    S(93, AF_PACKET, SOCK_PACKET, 0);
    S(94, AF_ALG, SOCK_STREAM, 0);
    S(95, AF_ALG, SOCK_DGRAM, 0);
    S(96, AF_ALG, SOCK_SEQPACKET, 0);
    S(97, AF_ALG, SOCK_RAW, 0);
    S(98, AF_ALG, SOCK_RDM, 0);
    S(99, AF_ALG, SOCK_PACKET, 0);

#define SP(n, f, ty, idx) case n: socketpair(f, ty, 0, pipes); fd = pipes[idx]; break
    SP(100, AF_UNIX, SOCK_STREAM, 0);
    SP(101, AF_UNIX, SOCK_STREAM, 1);
    SP(102, AF_UNIX, SOCK_DGRAM, 0);
    SP(103, AF_UNIX, SOCK_DGRAM, 1);
    SP(104, AF_UNIX, SOCK_SEQPACKET, 0);
    SP(105, AF_UNIX, SOCK_SEQPACKET, 1);

    default:
        // XXX nonblocking sockets?
        // XXX AF_NETLINK x (DGRAM,RAW) x protonr 0..22
        // XXX (INET, INET6) x SOCK_RAW x protonr 0..256
        // XXX PACKET X (DGRAM, RAW) x htons(rawtypes)
        // XXX weird files from /dev, /proc, /sys, etc..
        return -1;
    }
    return fd;
}

