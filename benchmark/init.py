#!/usr/bin/python
#author: jobs
#email: yao050421103@gmail.com
import sys
import os
import time
import json
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
    uri = argv[1]
    with open(uri) as f:
        data = json.load(f)['data']
    host_file = 'hosts'
    for key in data:
        if not os.path.exists(key):
            os.mkdir(key)
        if os.path.exists(host_file):
            shutil.copy(host_file, key)
        os.system('python gendata.py %d %s/data' % (data[key], key))
        os.system('python gencases.py %s %s' % (uri, key))
    return True

if __name__ == "__main__":
    if not init(sys.argv):
        manual()