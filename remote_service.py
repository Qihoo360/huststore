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
        python remote_service.py [user] [host_file] [prefix] [action]
        
        [action]
            --start                           start remote service
            --stop                            stop remote service
            
    sample:
        python remote_service.py jobs host.txt /opt/huststore/hustdb --start
        python remote_service.py jobs host.txt /opt/huststore/hustdb --stop
        """

def remote_ssh(key, option, user, host_file, cmds):
    cmd_file = '%s_%s.sh' % (key, datetime.datetime.now().strftime('%Y%m%d%H%M%S'))
    with open(cmd_file, 'w') as f:
        f.writelines(merge(cmds))
    if os.path.exists(cmd_file):
        os.system('python remote_ssh.py %s %s %s %s' % (option, user, host_file, cmd_file))
        os.remove(cmd_file)
    return True

def remote_service(user, host_file, prefix, action):
    if '--start' == action:
        cmd = 'sh start.sh'
    elif '--stop' == action:
        cmd = 'sh stop.sh'
    else:
        return False
    return remote_ssh('remote_service', '--slient', user, host_file, ['cd %s' % prefix, cmd])

def parse_shell(argv):
    size = len(argv)
    if size != 4:
        return False
    remote_service(argv[1], argv[2], argv[3], argv[4])
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()