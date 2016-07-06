#!/usr/bin/env python
"""
Generate syscall input files in the driver's file format.
"""
import struct, sys

BUFDELIM = "\xa5\xc9"
CALLDELIM = "\xb7\xe3"

class Buf(object) :
    def __init__(self) :
        self.buf = []
        self.pos = 0
    def add(self, x) :
        #print repr(self), 'add', x.encode('hex')
        self.buf.append(x)
    def pack(self, fmt, *args) :
        x = struct.pack(fmt, *args)
        self.add(x)
    def __str__(self) :
        return ''.join(self.buf)

class Num(object) :
    def __init__(self, v) :
        self.v = v
    def mkArg(self, buf, xtra) :
        buf.pack('!B', 0)
        buf.pack('!Q', self.v)
class Alloc(object) :
    def __init__(self, sz) :
        self.sz = sz
    def mkArg(self, buf, xtra) :
        buf.pack('!BI', 1, self.sz)
class String(object) :
    def __init__(self, v) :
        self.v = v
    def Len(self) :
        return Len(self)
    def mkArgTyp(self, typ, buf, xtra) :
        self.pos = xtra.pos
        xtra.pos += 1
        buf.pack('!B', typ)
        xtra.add(BUFDELIM)
        xtra.add(self.v)
    def mkArg(self, buf, xtra) :
        self.mkArgTyp(2, buf, xtra)
def StringZ(v) :
    return String(v + '\0')

class Len(object) :
    def mkArg(self, buf, xtra) :
        buf.pack('!B', 3)
class File(String) :
    def mkArg(self, buf, xtra) :
        self.mkArgTyp(4, buf, xtra)
class StdFile(object) :
    def __init__(self, v) :
        self.v = v
    def mkArg(self, buf, xtra) :
        buf.pack('!BH', 5, self.v)
class Vec64(object) :
    def __init__(self, *vs) :
        assert len(vs) < 256
        self.v = vs
    def mkArg(self, buf, xtra) :
        buf.pack('!BB', 7, len(self.v))
        for x in self.v :
            mkArg(buf, xtra, x)
class Filename(String) :
    def mkArg(self, buf, xtra) :
        self.mkArgTyp(8, buf, xtra)
class Pid(object) :
    def __init__(self, v) :
        self.v = v
    def mkArg(self, buf, xtra) :
        buf.pack('!BB', 9, self.v)
MyPid = Pid(0)
PPid = Pid(1)
ChildPid = Pid(2)

class Ref(object) :
    def __init__(self, nc, na) :
        self.nc, self.na = nc,na
    def mkArg(self, buf, xtra) :
        buf.pack('!BBB', 10, self.nc, self.na)

def mkArg(buf, xtra, x) :
    if isinstance(x, str) :
        x = StringZ(x)
    elif isinstance(x, int) or isinstance(x, long) :
        x = Num(x)
    x.mkArg(buf, xtra)

def mkSyscall(nr, *args) :
    args = list(args)
    while len(args) < 6 :
        args.append(0)

    buf = Buf()
    xtra = Buf()
    buf.pack('!H', nr)
    for n,arg in enumerate(args) :
        #print 'arg', n
        mkArg(buf, xtra, arg)
    return str(buf) + str(xtra)

def mkSyscalls(*calls) :
    r = []
    for call in calls :
        r.append(mkSyscall(*call))
    return CALLDELIM.join(r)

def writeFn(fn, buf) :
    with file(fn, 'w') as f :
        f.write(buf)

def test(fn) :
    # cleanup temp files made by driver
    subprocess.call("rm -rf /tmp/file?", shell=True)
    # hokey, but guarantees that fd=1 is not readable
    st = subprocess.call("./driver -tv < %s > /tmp/.xxx" % fn, shell=True)
    st = subprocess.call("egrep -q 'returned [^-]' /tmp/.xxx", shell=True)
    return st == 0
    
if __name__ == '__main__' :
    read = 0
    write = 1
    open = 2
    writev = 20
    execve = 59
    buf = Alloc(1024)
    l = Len()
    fd = File('HELLO WORLD!\n')

    ex1 = (write, 1, 'hello World!\n', l)
    writeFn('inputs/ex1', mkSyscalls(ex1))

    ex2 = (read, fd, buf, l)
    writeFn('inputs/ex2', mkSyscalls(ex2))

    writeFn('inputs/ex3', mkSyscalls(ex1, ex2))
    writeFn('inputs/ex4', mkSyscalls((read, StdFile(1), buf, l)))

    iov = Vec64('test\n', l, 'vec\n', l)
    writeFn('inputs/ex5', mkSyscalls((writev, 1, iov, 2)))
    
    writeFn('inputs/ex6', mkSyscalls((open, Filename("testing"), 0), (read, 3, buf, l)))

    writeFn('inputs/ex7', mkSyscalls((execve, Filename("#!/bin/sh\necho hi $FOO $@!\n"), Vec64("prog\0", "test\0", "this\0", 0), Vec64("FOO=bar\0", 0))))

