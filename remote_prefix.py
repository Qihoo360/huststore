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
        python remote_prefix.py [user] [host_file] [prefix] [owner]
    sample:
        python remote_prefix.py jobs host.txt /opt/huststore jobs
        """

def remote_ssh(key, option, user, host_file, cmds):
    cmd_file = '%s_%s.sh' % (key, datetime.datetime.now().strftime('%Y%m%d%H%M%S'))
    with open(cmd_file, 'w') as f:
        f.writelines(merge(cmds))
    if os.path.exists(cmd_file):
        os.system('python remote_ssh.py %s %s %s %s' % (option, user, host_file, cmd_file))
        os.remove(cmd_file)
    return True

def remote_prefix(user, host_file, prefix, owner):
    return remote_ssh('remote_prefix', '', user, host_file, [
        'test -d %s || sudo -S mkdir -p %s' % (prefix, prefix),
        'sudo chown -R %s:%s %s' % (owner, owner, prefix)
        ])

def parse_shell(argv):
    size = len(argv)
    if size != 5:
        return False
    return remote_prefix(argv[1], argv[2], argv[3], argv[4])

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()