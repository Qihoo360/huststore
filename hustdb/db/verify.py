#/usr/bin/python
import sys
import os
import time
import datetime
import json
import string
import requests
import random
import shutil
import struct
import base64
from time import sleep

USER = 'huststore'
PASSWD = 'huststore'

SIZE_LEN = 4
BLOCK = 512

def manual(): 
    print """
    usage:
        python verify.py [option]
            [option]
                host: ip or domain
    sample:
        python verify.py 192.168.1.101
        """

def fetch_keys(func, host, block):
    def __get_keys(host, file, block):
        offset = 0
        items = []
        all_keys = []
        while 1:
            cmd = '%s/hustdb/keys?offset=%d&size=%d&file=%d&start=0&end=1024' % (
                host, offset, block, file)
            r = func(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '[file-%d] %d' % (file, r.status_code)
                break
            #[{"key":"aHVzdGRiaGFrZXk=","ver":1}]
            keys = [base64.b64decode(item['key']) for item in filter(lambda item: 'ver' in item and item['ver'] > 0, json.loads(r.content))]
            if len(keys) < 1:
                break
            #record(keys)
            all_keys.extend(keys)
            __size = int(r.headers['keys'])
            offset = offset + len(keys)
            if __size < block:
                break
        return all_keys
    def __fetch_keys(func, host, block):
        cmd = '%s/hustdb/file_count' % host
        r = func(cmd, auth=(USER, PASSWD))
        file_count = int(r.content) if 200 == r.status_code else 0
        keys = []
        for fid in range(file_count):
            keys.extend(__get_keys(host, fid, block))
        return keys
    return __fetch_keys(func, host, block)

def fetch(sess, host, block):
    def __fetch_keys(func, host, stat, block):
        number_of_keys = stat['size']
        if number_of_keys < 1:
            return
        return fetch_keys(func, host, block)
    def __fetch(func, host, block):
        cmd = '%s/hustdb/stat_all' % host
        r = func(cmd, auth=(USER, PASSWD))
        stats = r.json()
        return __fetch_keys(func, host, stats[0], block)
    return __fetch(sess.get, host, block)

def set_keys(sess, host, keys):
    for key in keys:
        cmd = '%s/hustdb/put?key=%s' % (host, key)
        r = sess.post(cmd, key, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
            return False
    return True

def del_keys(sess, host, keys):
    for key in keys:
        cmd = '%s/hustdb/del?key=%s' % (host, key)
        r = sess.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)

def verify_keys(sess, host, block, keys):
    dbkeys = fetch(sess, host, block)
    err = False
    for key in keys:
        if not key in dbkeys:
            err = True
            print '"%s" missing' % key
    if not err:
        print 'verify ok!'
    else:
        with open('keys.dump', 'w') as f:
            f.write('[generated keys]')
            f.write('\n')
            for key in keys:
                f.write(key)
                f.write('\n')
            f.write('[stored keys]')
            for key in dbkeys:
                f.write(key)
                f.write('\n')

def verify(host, block):
    sess = requests.Session()
    now = time.time()
    keys = ['%d|test_str|%d' % (now, now + i) for i in xrange(32)]
    if not set_keys(sess, host, keys):
        return
    verify_keys(sess, host, block, keys)
    del_keys(sess, host, keys)
    
def get_host(argv):
    size = len(argv)
    if 2 == size:
        return 'http://%s' % argv[1]
    return 'http://localhost:8085'
                
if __name__ == "__main__":
    verify(get_host(sys.argv), BLOCK)