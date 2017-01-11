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
        python remote_deploy.py [user] [host_file] [prefix] [tar]
    sample:
        python remote_deploy.py jobs host.txt /opt/huststore elf_hustdb.tar.gz
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

def remote_deploy(user, host, prefix, tar):
    return merge([
        '%s %s %s@%s:/tmp;' % (get_cmd(user, 'scp'), tar, user, host),
        gen_ssh_cmd(user, host, [
            'mv /tmp/%s %s' % (tar, prefix),
            'cd %s' % prefix,
            'tar -zxf %s -C .' % tar,
            'rm -f %s' % tar,
            ])
        ])

def parse_shell(argv):
    size = len(argv)
    if size != 5:
        return False
    (user, host_file, prefix, tar) = (argv[1], argv[2], argv[3], argv[4])
    for host in get_items(host_file):
        print host
        os.system(remote_deploy(user, host, prefix, tar))
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()