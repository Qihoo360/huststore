# -*- coding: UTF-8 -*-
#/usr/bin/python
import os
import sys
import string
import datetime

merge = lambda l: string.join(l, '\n')

def manual(): 
    print """
    usage:
        python remote_service.py [port] [user] [ppk] [host_file] [bin_folder] [action]
        
        [action]
            --start                           start remote service
            --stop                            stop remote service
            
    sample:
        python remote_service.py 22 jobs jobs.ppk host.txt /opt/huststore/hustdb --start
        python remote_service.py 22 jobs jobs.ppk host.txt /opt/huststore/hustdb --stop
        """

def remote_ssh(key, port, user, ppk, host_file, cmds):
    cmd_file = '%s_%s.sh' % (key, datetime.datetime.now().strftime('%Y%m%d%H%M%S'))
    with open(cmd_file, 'w') as f:
        f.writelines(merge(cmds))
    if os.path.exists(cmd_file):
        os.system('python remote_ssh.py %s %s %s %s %s' % (port, user, ppk, host_file, cmd_file))
        os.remove(cmd_file)
    return True

def remote_service(port, user, ppk, host_file, bin_folder, action):
    if '--start' == action:
        cmd = 'sh start.sh'
    elif '--stop' == action:
        cmd = 'sh stop.sh'
    else:
        return False
    return remote_ssh('remote_service', port, user, ppk, host_file, ['cd %s' % bin_folder, cmd])

def parse_shell(argv):
    return remote_service(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]) if 7 == len(argv) else False

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()