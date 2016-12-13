#!/usr/bin/python
#author: jobs
#email: yao050421103@gmail.com
import sys
import os
import time
import shutil

def manual(): 
    print """
    usage:
        python init.py [conf]
    sample:
        python init.py wrk.json
        """

def init(argv):
    size = len(argv)
    if 2 != size:
        return False
    items = [
        ['256B', '256'], 
        ['1KB', '1024'], 
        ['4KB', '4096'], 
        ['16KB', '16384'], 
        ['64KB', '65536']
        ]
    DIR = 0
    BYTES = 1
    host_file = 'hosts'
    for item in items:
        if not os.path.exists(item[DIR]):
            os.mkdir(item[DIR])
        if os.path.exists(host_file):
            shutil.copy(host_file, item[DIR])
        os.system('python gendata.py %s %s/data' % (item[BYTES], item[DIR]))
        os.system('python gencases.py %s %s' % (argv[1], item[DIR]))
    return True

if __name__ == "__main__":
    if not init(sys.argv):
        manual()