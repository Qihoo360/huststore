#!/usr/bin/python
# author: jobs
# email: yao050421103@163.com
import os
import sys
import string
import datetime

merge = lambda l: string.join(l, '\n')

def manual(): 
    print """
    usage:
        python remote_status.py [host_file] [port]
    sample:
        python remote_status.py host.txt 8082
        """

def get_items(uri):
    with open(uri) as f:
        return filter(lambda s: len(s) > 0 and not s.startswith('#'), map(
            lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))

def remote_status(host_file, port):
    for host in get_items(host_file):
        print '[%s:%s]' % (host, port)
        os.system('curl --silent "%s:%s/status.html"' % (host, port))
    return True

def parse_shell(argv):
    return remote_status(argv[1], argv[2]) if 3 == len(argv) else False

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()