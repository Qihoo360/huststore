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
    
def split_key(key):
    pos = (key[0] + key[1]) / 2
    return [[key[0], pos], [pos, key[1]]]

def gen_keys(size):
    keys = [[0, 512], [512, 1024]]
    size = size - 2
    index = 0
    while size > 0:
        tmp_keys = keys[:index]
        tmp_keys.extend(split_key(keys[index]))
        if index < len(keys) - 1:
            tmp_keys.extend(keys[(index + 1):])
            keys = tmp_keys
            index = index + 2
        else:
            keys = tmp_keys
            index = 0
        size = size - 1
    return keys
    
def gen(argv):
    size = len(argv)
    if 3 != size:
        return False
    hosts = get_hosts(argv[1])
    size = len(hosts)
    if size < 2:
        print 'the number of host should be 2 at least'
        return False
    sections = range(size)
    keys = gen_keys(size)
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