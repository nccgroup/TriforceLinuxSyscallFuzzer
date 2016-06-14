#!/usr/bin/env python
"""
Generate input files with different shapes for each system call.
"""

import os
from gen import *

defaultBuf = Alloc(8)
defaultStr = '12345678'

argMap = {
    'buffer': defaultBuf,
    'fd': StdFile(1),
    'filename': Filename('testing'),
    'int': 0,
    'ptr': defaultBuf,
    'signalno': 0,
    'string': defaultStr,
    'len': Len(),
}

def parseArg(arg, stdFile=False):
    if arg == 'fd' and not stdFile:
        return File('testing')
    return argMap.get(arg, defaultBuf)

def main() :
    try:
        os.mkdir('gen2-inputs')
    except OSError:
        pass

    already = set()
    for callnr in xrange(360) :
        i = 0
        with open('gen2-shapes.txt', 'r') as f:
            for line in f:
                for stdFile in (True, False):
                    args = [parseArg(arg, stdFile) for arg in line.strip().split(',')]
                    buf = mkSyscall(callnr, *args)
                    if not buf in already:
                        writeFn('gen2-inputs/call%03d-%d' % (callnr, i), buf)
                        i += 1
                        already.add(buf)

if __name__ == '__main__' :
    main()
