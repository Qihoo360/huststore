# -*- coding: UTF-8 -*-
#/usr/bin/python
import os
import string
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

def manual(): 
    print """
    usage:
        python remote_scp.py [port] [user] [ppk] [host_file] [remote_folder] [local_file1] [local_file2] ... 
    sample:
        python remote_scp.py 22 jobs jobs.ppk host.txt /tmp file1 file2
        """

def get_items(uri):
    with open(uri) as f:
        return filter(lambda s: len(s) > 0 and not s.startswith('#'), map(
            lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))

def scp(port, ppk, user, files, host, prefix):
    return 'scp -P %s -i %s %s %s@%s:%s;' % (port, ppk, string.join(files, ' '), user, host, prefix)

def parse_shell(argv):
    size = len(argv)
    if size < 7:
        return False
    (port, user, ppk, host_file, prefix, files) = (argv[1], argv[2], argv[3], argv[4], argv[5], argv[6:])
    for host in get_items(host_file):
        print host
        cmd = scp(port, ppk, user, files, host, prefix)
        os.system(cmd)
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()