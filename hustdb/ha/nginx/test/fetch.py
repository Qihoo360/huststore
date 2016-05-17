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
gentm = lambda: datetime.datetime.now().strftime('[%Y-%m-%d %H:%M:%S] ')

SIZE_LEN = 4
BLOCK = 512

def manual(): 
    print """
    usage:
        python fetch.py [option]
            [option]
                host: ip or domain
    sample:
        python fetch.py loop
        python fetch.py 192.168.1.101
        """

def log_err(str):
    print str
    with open('fetch.log', 'a+') as f:
        f.writelines('%s%s\n' % (gentm(), str))

def get_count(func, host, key):
    cmd = '%s/%s' % (host, key)
    r = func(cmd, auth=(USER, PASSWD))
    return int(r.content) if 200 == r.status_code else 0
    
def stat_all(func, host, peer):
    cmd = '%s/stat_all?peer=%d' % (host, peer)
    r = func(cmd, auth=(USER, PASSWD))
    return r.json() if 200 == r.status_code else None

def fetch_keys(debug, func, host, peer, block, f):
    def __get_keys(debug, host, peer, file, block, record):
        offset = 0
        items = []
        while 1:
            cmd = '%s/keys?peer=%d&offset=%d&size=%d&file=%d&start=0&end=1024' % (
                host, peer, offset, block, file)
            r = func(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '[peer-%d][file-%d] %d' % (peer, file, r.status_code)
                break
            if debug:
                print 'peer: %d, file: %d, keys: %s' % (peer, file, r.headers['keys'])
            #[{"key":"aHVzdGRiaGFrZXk=","ver":1}]
            keys = [base64.b64decode(item['key']) for item in filter(lambda item: 'ver' in item and item['ver'] > 0, json.loads(r.content))]
            if len(keys) < 1:
                break
            record(keys)
            __size = int(r.headers['keys'])
            offset = offset + len(keys)
            if __size < block:
                break
        return offset
    def __fetch_keys(debug, func, host, peer, block, f):
        def __record(peer, fid, f):
            def __record_imp(keys):
                f.write('[file %d]\n' % fid)
                for key in keys:
                    f.write(key)
                    f.write('\n')
                f.write('\n')
            return __record_imp
        file_count = get_count(func, host, 'file_count?peer=%d' % peer)
        return sum(map(lambda fid: __get_keys(
            debug, host, peer, fid, block, __record(peer, fid, f)
            ), [fid for fid in range(file_count)]))
    return __fetch_keys(debug, func, host, peer, block, f)

def fetch_tbkeys(debug, func, host, key, peer, tb, block, f):
    def __get_tbkeys(debug, func, host, key, peer, tb, block, record):
        offset = 0
        while 1:
            cmd = '%s/%s?peer=%d&tb=%s&offset=%d&size=%d' % (host, key, peer, tb, offset, block)
            r = func(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '[peer-%d][tb-%s] %d' % (peer, tb, r.status_code)
                break
            if debug:
                print 'peer: %d, tb: %s, keys: %s' % (peer, tb, r.headers['keys'])
            # [{"key":"aHVzdGRiaGFrZXk=","ver":1,"tb":"hustdbhatb"}]
            keys = [base64.b64decode(item['key']) for item in filter(lambda item: 'ver' in item and item['ver'] > 0, json.loads(r.content))]
            record(keys)
            __size = int(r.headers['keys'])
            offset = offset + __size
            if __size < block:
                break
        return offset
    def __fetch_tbkeys(debug, func, host, key, peer, tb, block, f):
        def __record(peer, tb, f):
            def __record_imp(keys):
                f.write('[table %s]\n' % tb)
                for key in keys:
                    f.write(key)
                    f.write('\n')
                f.write('\n')
            return __record_imp
        return __get_tbkeys(debug, func, host, key, peer, tb, block, __record(peer, tb, f))
    return __fetch_tbkeys(debug, func, host, key, peer, tb, block, f)

def fetch_zkeys_byscore(debug, func, host, tb, block, min, max, f):
    def __get_zkeys(debug, func, host, tb, block, min, max, record):
        offset = 0
        while 1:
            cmd = '%s/zrangebyscore?tb=%s&offset=%d&size=%d&min=%d&max=%d' % (host, tb, offset, block, min, max)
            r = func(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '[tb-%s] %d' % (tb, r.status_code)
                break
            if debug:
                print 'tb: %s, keys: %s' % (tb, r.headers['keys'])
            # [{"key":"aHVzdGRiaGFrZXk=","ver":1,"tb":"hustdbhatb"}]
            keys = [base64.b64decode(item['key']) for item in filter(lambda item: 'ver' in item and item['ver'] > 0, json.loads(r.content))]
            record(keys)
            __size = int(r.headers['keys'])
            offset = offset + __size
            if __size < block:
                break
        return offset
    def __fetch_zkeys(debug, func, host, tb, block, min, max, f):
        def __record(tb, f):
            def __record_imp(keys):
                f.write('[table %s]\n' % tb)
                for key in keys:
                    f.write(key)
                    f.write('\n')
                f.write('\n')
            return __record_imp
        return __get_zkeys(debug, func, host, tb, block, min, max, __record(tb, f))
    return __fetch_zkeys(debug, func, host, tb, block, min, max, f)

def fetch_zkeys_byrank(debug, func, host, tb, block, f):
    def __get_zkeys(debug, func, host, tb, block, record):
        offset = 0
        while 1:
            cmd = '%s/zrangebyrank?tb=%s&offset=%d&size=%d' % (host, tb, offset, block)
            r = func(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '[tb-%s] %d' % (tb, r.status_code)
                break
            if debug:
                print 'tb: %s, keys: %s' % (tb, r.headers['keys'])
            # [{"key":"aHVzdGRiaGFrZXk=","ver":1,"tb":"hustdbhatb"}]
            keys = [base64.b64decode(item['key']) for item in filter(lambda item: 'ver' in item and item['ver'] > 0, json.loads(r.content))]
            record(keys)
            __size = int(r.headers['keys'])
            offset = offset + __size
            if __size < block:
                break
        return offset
    def __fetch_zkeys(debug, func, host, tb, block, f):
        def __record(tb, f):
            def __record_imp(keys):
                f.write('[table %s]\n' % tb)
                for key in keys:
                    f.write(key)
                    f.write('\n')
                f.write('\n')
            return __record_imp
        return __get_zkeys(debug, func, host, tb, block, __record(tb, f))
    return __fetch_zkeys(debug, func, host, tb, block, f)

def fetch(debug, host, block):
    def __fetch_keys(debug, func, host, peer, stat, block, output):
        with open(os.path.join(output, 'peer-%d.keys' % peer), 'a+') as f:
            number_of_keys = stat['size']
            if number_of_keys < 1:
                return
            size = fetch_keys(debug, func, host, peer, block, f)
            if size != number_of_keys:
                print '[peer-%d] number_of_keys: %d, size: %d' % (peer, number_of_keys, size)
    def __fetch_tb_keys(debug, func, host, key, peer, stats, block, output):
        with open(os.path.join(output, 'peer-%d.%s' % (peer, key)), 'a+') as f:
            for stat in stats:
                tb = stat['table']
                number_of_tbs = stat['size']
                size = fetch_tbkeys(debug, func, host, key, peer, tb, block, f)
                if size != number_of_tbs:
                    print '[peer-%d] number of tbs: %d, size: %d' % (peer, number_of_tbs, size)
    def __fetch_zkeys_byrank(debug, func, host, stats, block, output):
        with open(os.path.join(output, 'zrangebyrank'), 'a+') as f:
            for stat in stats:
                tb = stat['table']
                number_of_tbs = stat['size']
                size = fetch_zkeys_byrank(debug, func, host, tb, block, f)
                if size != number_of_tbs:
                    print 'number of tbs: %d, size: %d' % (number_of_tbs, size)
    def __fetch_zkeys_byscore(debug, func, host, stats, block, min, max, output):
        with open(os.path.join(output, 'zrangebyscore'), 'a+') as f:
            for stat in stats:
                tb = stat['table']
                number_of_tbs = stat['size']
                size = fetch_zkeys_byscore(debug, func, host, tb, block, min, max, f)
                if size != number_of_tbs:
                    print 'number of tbs: %d, size: %d' % (number_of_tbs, size)
    def __extend_zset(stats, zset):
        for item in filter(lambda stat: 'Z' == stat['type'], stats): 
            found = False
            for elem in zset:
                if item['table'] == elem['table']:
                    found = True
            if not found:
                zset.append(item)
    def __fetch(debug, func, host, block, output):
        zset = []
        for peer in xrange(get_count(sess.get, host, 'peer_count')):
            print 'fetch peer %d...' % peer
            stats = stat_all(func, host, peer)
            __extend_zset(stats, zset)
            __fetch_keys(debug, func, host, peer, stats[0], block, output)
            __fetch_tb_keys(debug, func, host, 'hkeys', peer, filter(
                lambda stat: 'H' == stat['type'], stats), block, output)
            __fetch_tb_keys(debug, func, host, 'smembers', peer, filter(
                lambda stat: 'S' == stat['type'], stats), block, output)
        __fetch_zkeys_byrank(debug, func, host, zset, block, output)
        __fetch_zkeys_byscore(debug, func, host, zset, block, 0, 100, output)
    output = os.path.join(os.path.abspath('.'), 'keys')
    if os.path.exists(output):
        shutil.rmtree(output)
    if not os.path.exists(output):
        os.mkdir(output)
    sess = requests.Session()
    __fetch(debug, sess.get, host, block, output)
    print 'fetch over'
    return True
    
def get_host(argv):
    size = len(argv)
    if 2 == size:
        return 'http://%s' % argv[1]
    return 'http://localhost:8082'
                
if __name__ == "__main__":
    fetch(True, get_host(sys.argv), BLOCK)