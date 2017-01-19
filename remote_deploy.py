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
        python remote_deploy.py [option] [user] [host_file] [prefix] [tar]
        
        [option]
            --silent                          run in silent mode
        
    sample:
        python remote_deploy.py jobs host.txt /opt/huststore elf_hustdb.tar.gz
        python remote_deploy.py --silent jobs host.txt /opt/huststore elf_hustdb.tar.gz
        """

def remote_ssh(key, option, user, host_file, cmds):
    cmd_file = '%s_%s.sh' % (key, datetime.datetime.now().strftime('%Y%m%d%H%M%S'))
    with open(cmd_file, 'w') as f:
        f.writelines(merge(cmds))
    if os.path.exists(cmd_file):
        os.system('python remote_ssh.py %s %s %s %s' % (option, user, host_file, cmd_file))
        os.remove(cmd_file)
    return True

def remote_deploy(option, user, host_file, prefix, tar):
    os.system('python remote_scp.py %s %s %s /tmp %s' % (option, user, host_file, tar))
    return remote_ssh('remote_deploy', option, user, host_file, [
        'mv /tmp/%s %s' % (tar, prefix),
        'cd %s' % prefix,
        'tar -zxf %s -C .' % tar,
        'rm -f %s' % tar
        ])

def parse_shell(argv):
    size = len(argv)
    if size < 5 or size > 6:
        return False
    silent = True if '--silent' == argv[1] else False
    idx = 2 if silent else 1
    option = '--silent' if silent else ''
    return remote_deploy(option, argv[idx], argv[idx + 1], argv[idx + 2], argv[idx + 3])

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()