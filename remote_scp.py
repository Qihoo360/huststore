#!/usr/bin/python
# author: jobs
# email: yao050421103@163.com
import os
import sys
import string

merge = lambda l: string.join(l, '\n')
get_cmd = lambda user, cmd: 'sudo -u %s %s -oStrictHostKeyChecking=no' % (user, cmd)

def manual(): 
    print """
    usage:
        python remote_scp.py [user] [host_file] [remote_folder] [local_file1] [local_file2] ...
    sample:
        python remote_scp.py jobs host.txt /opt/huststore/hustdbha/conf nginx.conf hustdbtable.json
        """

def get_items(uri):
    with open(uri) as f:
        return filter(lambda s: len(s) > 0 and not s.startswith('#'), map(
            lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))

def remote_scp(user, files, host, prefix):
    return '%s %s %s@%s:%s;' % (get_cmd(user, 'scp'), string.join(files, ' '), user, host, prefix)

def parse_shell(argv):
    size = len(argv)
    if size < 5:
        return False
    (user, host_file, prefix, files) = (argv[1], argv[2], argv[3], argv[4:])
    for host in get_items(host_file):
        print host
        os.system(remote_scp(user, files, host, prefix))
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()