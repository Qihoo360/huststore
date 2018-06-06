# -*- coding: UTF-8 -*-
#/usr/bin/python
import os
import string
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

merge = lambda l: string.join(l, '\n')

def manual(): 
    print """
    usage:
        python remote_ssh.py [port] [user] [ppk] [host_file] [cmds_file]
    sample:
        python remote_ssh.py 22 jobs jobs.ppk host.txt cmds.txt
        """

def get_items(uri):
    with open(uri) as f:
        return filter(lambda s: len(s) > 0 and not s.startswith('#'), map(
            lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))

def ssh(port, ppk, user, host, cmds):
    return merge([
        'ssh -p %s -i %s %s@%s \\' % (port, ppk, user, host),
        '    \' \\',
        merge(['    %s; \\' % cmd for cmd in cmds]),
        '    \''
        ])

def parse_shell(argv):
    size = len(argv)
    if size != 6:
        return False
    (port, user, ppk, host_file, cmds_file) = (argv[1], argv[2], argv[3], argv[4], argv[5])
    for host in get_items(host_file):
        print host
        cmd = ssh(port, ppk, user, host, get_items(cmds_file))
        os.system(cmd)
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()