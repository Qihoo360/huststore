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
        python remote_service.py [option] [user] [host_file] [bin_folder] [action]
        
        [option]
            --silent                          run in silent mode
        
        [action]
            --start                           start remote service
            --stop                            stop remote service
            
    sample:
        python remote_service.py jobs host.txt /opt/huststore/hustdb --start
        python remote_service.py jobs host.txt /opt/huststore/hustdb --stop
        
        python remote_service.py --silent jobs host.txt /opt/huststore/hustdb --start
        python remote_service.py --silent jobs host.txt /opt/huststore/hustdb --stop
        """

def remote_ssh(key, option, user, host_file, cmds):
    cmd_file = '%s_%s.sh' % (key, datetime.datetime.now().strftime('%Y%m%d%H%M%S'))
    with open(cmd_file, 'w') as f:
        f.writelines(merge(cmds))
    if os.path.exists(cmd_file):
        os.system('python remote_ssh.py %s %s %s %s' % (option, user, host_file, cmd_file))
        os.remove(cmd_file)
    return True

def remote_service(option, user, host_file, bin_folder, action):
    if '--start' == action:
        cmd = 'sh start.sh'
    elif '--stop' == action:
        cmd = 'sh stop.sh'
    else:
        return False
    return remote_ssh('remote_service', option, user, host_file, ['cd %s' % bin_folder, cmd])

def parse_shell(argv):
    size = len(argv)
    if size < 5 or size > 6:
        return False
    silent = True if '--silent' == argv[1] else False
    idx = 2 if silent else 1
    option = '--silent' if silent else ''
    return remote_service(option, argv[idx], argv[idx + 1], argv[idx + 2], argv[idx + 3])

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()