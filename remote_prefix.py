#!/usr/bin/python
# author: jobs
# email: yao050421103@163.com
import os
import sys
import string

merge = lambda l: string.join(l, '\n')

def manual(): 
    print """
    usage:
        python remote_prefix.py [user] [host_file] [prefix] [owner]
    sample:
        python remote_prefix.py jobs host.txt /opt/huststore jobs
        """

def get_items(uri):
    with open(uri) as f:
        return filter(lambda s: len(s) > 0 and not s.startswith('#'), map(
            lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))

def gen_ssh_cmd(user, host, cmds):
    return merge([
        'ssh %s@%s \\' % (user, host),
        '    \' \\',
        merge(['    %s; \\' % cmd for cmd in cmds]),
        '    \''
        ])

def remote_prefix(user, host, prefix, owner):
    return gen_ssh_cmd(user, host, [
        'test -d %s || sudo -S mkdir -p %s' % (prefix, prefix),
        'sudo chown -R %s:%s %s' % (owner, owner, prefix)
        ])

def parse_shell(argv):
    size = len(argv)
    if size != 5:
        return False
    (user, host_file, prefix, owner) = (argv[1], argv[2], argv[3], argv[4])
    for host in get_items(host_file):
        print host
        os.system(remote_prefix(user, host, prefix, owner))
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()