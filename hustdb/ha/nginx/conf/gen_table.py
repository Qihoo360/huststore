#!/usr/bin/python
# email: yao050421103@gmail.com
import sys
import os.path
import datetime
import string
import json

merge = lambda l: string.join(l, '\n')

def manual(): 
    print """
    usage:
        python gen_table.py [host_file] [output]
    sample:
        python gen_table.py hosts hustdbtable.json
        """

def get_hosts(path):
    f = open(path, 'r')
    hosts = filter(lambda s: len(s) > 0, map(lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))
    f.close()
    return hosts
    
def gen(argv):
    size = len(argv)
    if 3 != size:
        return False
    hosts = get_hosts(argv[1])
    size = len(hosts)
    sections = range(size)
    delta = int(1024 / size)
    keys = [[
        1024 - delta * (section + 1), 
        1024 - delta * (section)
        ] if sections.index(section) < size - 1 else [
        0, 1024 - delta * (section)
        ] for section in sections]
    keys.reverse()
    host_list = [[
        host, hosts[hosts.index(host) + 1]
        ] if hosts.index(host) < size - 1 else [
        host, hosts[0]
        ] for host in hosts]
    table = {'table': []}
    for section in sections:
        table['table'].append({ "item": { "key": keys[section], "val": host_list[section] } })
    with open(argv[2], 'w') as f:
        json.dump(table, f, indent=4)
    return True

if __name__ == "__main__":
    if not gen(sys.argv):
        manual()