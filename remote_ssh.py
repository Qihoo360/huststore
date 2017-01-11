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
        python remote_ssh.py [option] [user] [host_file] [cmds_file]
        
        [option]
            -s : run in silent mode

    sample:
        python remote_ssh.py jobs host.txt cmds.txt
        python remote_ssh.py -s jobs host.txt cmds.txt
        """

def get_items(uri):
    with open(uri) as f:
        return filter(lambda s: len(s) > 0 and not s.startswith('#'), map(
            lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))

def remote_ssh(silent, user, host, cmds):
    return merge([
        '%s %s@%s \\' % (get_cmd(user, 'ssh') if silent else 'ssh', user, host),
        '    \' \\',
        merge(['    %s; \\' % cmd for cmd in cmds]),
        '    \''
        ])

def parse_shell(argv):
    size = len(argv)
    if size < 4 or size > 5:
        return False
    silent = True if '-s' == argv[1] else False
    idx = 2 if silent else 1
    (user, host_file, cmds_file) = (argv[idx], argv[idx + 1], argv[idx + 2])
    for host in get_items(host_file):
        print host
        os.system(remote_ssh(silent, user, host, get_items(cmds_file)))
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()