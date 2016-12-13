#!/usr/bin/python
#author: jobs
#email: yao050421103@gmail.com
import sys
import os
import time
import random

def manual(): 
    print """
    usage:
        python gendata.py [bytes] [output]
    sample:
        python gendata.py 256 256B/data
        """

MATRIX = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

def gen_char(idx):
    end = len(MATRIX) - 1
    if 0 == idx:
        end = end - 10
    return MATRIX[random.randint(0, end)]

def gen_line(size):
    return ''.join([gen_char(i) for i in xrange(size)])

def gen(bytes, output):
    with open(output, 'w') as f:
        f.write(gen_line(bytes))
    return True

def parse(argv):
    random.seed(time.time())
    args = len(argv)
    if args != 3:
        return False
    return gen(int(argv[1]), argv[2])

if __name__ == "__main__":
    if not parse(sys.argv):
        manual()