#!/usr/bin/env python3

from binascii import hexlify
import sys
import tempfile
import os
import subprocess
from random import randint as rand

def do_python(data, key):
    return bytes([b^key[i%len(key)] for i, b in enumerate(data)])

def do_fastxor(data, key, buffered=False):
    tmp_out = tempfile.mktemp()
    with tempfile.NamedTemporaryFile() as temp:
        temp.write(data)
        temp.flush()
        args = ['./fastxor', '-x', hexlify(key).decode("ascii"), temp.name, tmp_out]
        if buffered:
            args.append('-b')
        subprocess.run(args)
    res = open(tmp_out, 'rb').read()
    os.unlink(tmp_out)
    return res

def do_fastxor_file(data, key, buffered=False):
    tmp_out = tempfile.mktemp()
    with tempfile.NamedTemporaryFile() as keyfile:
        keyfile.write(key)
        keyfile.flush()
        with tempfile.NamedTemporaryFile() as temp:
            temp.write(data)
            temp.flush()
            args = ['./fastxor', '-f', keyfile.name, temp.name, tmp_out]
            if buffered:
                args.append('-b')
            subprocess.run(args)
    res = open(tmp_out, 'rb').read()
    os.unlink(tmp_out)
    return res

def do_test(data, key):
    py = do_python(data, key)
    # Max arg length on command line can cause problems
    if(len(key) < 10000):
        fx = do_fastxor(data, key)
    else:
        fx = py
    fxf = do_fastxor_file(data, key)
    print("%r\t%r\t%6d data\t%6d key" % (py==fx, py==fxf, len(data), len(key), ))
    if py != fx or py != fxf:
        sys.exit(1)
    if(len(key) < 10000):
        fx = do_fastxor(data, key, True)
    else:
        fx = py
    fxf = do_fastxor_file(data, key, True)
    print("%r\t%r\t%6d data\t%6d key (buffered)" % (py==fx, py==fxf, len(data), len(key), ))
    if py != fx or py != fxf:
        sys.exit(1)


do_test(b"a", b"1")
do_test(b"a", b"123")
do_test(b"abcd", b"123")
do_test(b"abcd", b"1234")
do_test(b"abcd"*8, b"123")
for i in range(0, 100):
    # test with shorted key length (to avoid max cmd line length pbs)
    data = os.urandom(rand(1, 1000000))
    do_test(data, os.urandom(rand(1, 10000)))
    # and long, for file
    key = os.urandom(rand(1, 1000000))
    do_test(data, key)
    do_test(key, data)
