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
        python remote_scp.py [option] [user] [host_file] [remote_folder] [local_file1] [local_file2] ...
        
        [option]
            -s : run in silent mode

    sample:
        python remote_scp.py jobs host.txt /opt/huststore/hustdbha/conf nginx.conf hustdbtable.json
        python remote_scp.py -s jobs host.txt /opt/huststore/hustdbha/conf nginx.conf hustdbtable.json
        """

def get_items(uri):
    with open(uri) as f:
        return filter(lambda s: len(s) > 0 and not s.startswith('#'), map(
            lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))

def remote_scp(silent, user, files, host, prefix):
    return '%s %s %s@%s:%s;' % (get_cmd(user, 'scp') if silent else 'scp', string.join(files, ' '), user, host, prefix)

def parse_shell(argv):
    size = len(argv)
    if size < 5:
        return False
    silent = True if '-s' == argv[1] else False
    idx = 2 if silent else 1
    (user, host_file, prefix, files) = (argv[idx], argv[idx + 1], argv[idx + 2], argv[(idx + 3):])
    for host in get_items(host_file):
        print host
        os.system(remote_scp(silent, user, files, host, prefix))
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()