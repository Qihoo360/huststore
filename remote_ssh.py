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
        python remote_ssh.py [user] [host_file] [cmds_file]
    sample:
        python remote_ssh.py jobs host.txt cmds.txt
        """

def get_items(uri):
    with open(uri) as f:
        return filter(lambda s: len(s) > 0 and not s.startswith('#'), map(
            lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))

def gen_ssh_cmd(user, host, cmds):
    return merge([
        '%s %s@%s \\' % (get_cmd(user, 'ssh'), user, host),
        '    \' \\',
        merge(['    %s; \\' % cmd for cmd in cmds]),
        '    \''
        ])
def remote_ssh(user, host, cmds):
    return gen_ssh_cmd(user, host, cmds)

def parse_shell(argv):
    size = len(argv)
    if size != 4:
        return False
    (user, host_file, cmds_file) = (argv[1], argv[2], argv[3])
    for host in get_items(host_file):
        print host
        os.system(remote_ssh(user, host, get_items(cmds_file)))
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()