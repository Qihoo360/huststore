#/usr/bin/python
import sys
import os
import time
import datetime
import json
import string
import requests
import random
import struct
from time import sleep

LOOPS = 10000
USER = 'huststore'
PASSWD = 'huststore'
gentm = lambda: datetime.datetime.now().strftime('[%Y-%m-%d %H:%M:%S] ')
test_cases = map(lambda i: { 'tb': 'hustdbhatb%d' % i, 'key': 'hustdbhakey%d' % i, 'val': 'hustdbhaval%d' % i }, xrange(1, 31))
gen_char = lambda i: str('\n' if 0 != i and 0 == i % 80 else random.randint(0, 9))
gen_body = lambda n: map(gen_char, xrange(n))
TIMEOUT = 60

SIZE_LEN = 4

def manual(): 
    print """
    usage:
        python autotest [uri] [action]
            [uri]
                uri: ip:port
            [action]
                put | get | get2 | del | exist |
                hset | hget | hget2 |hdel | hexist |
                sadd | srem | sismember |
                zadd | zrem | zismember | zscore | zscore2 |
                stat_all | sync_status | get_table | loop
    sample:
        python autotest.py localhost:8082 loop
        python autotest.py localhost:8082 stat_all
        python autotest.py localhost:8082 sync_status
        """

def log_err(path, str):
    print str
    with open(path, 'a+') as f:
        f.writelines('%s%s\n' % (gentm(), str))

def get_count(func, host, key):
    cmd = '%s/%s' % (host, key)
    r = func(cmd, auth=(USER, PASSWD))
    return int(r.content) if 200 == r.status_code else 0
    
def stat_all(func, host, peer):
    cmd = '%s/stat_all?peer=%d' % (host, peer)
    r = func(cmd, auth=(USER, PASSWD))
    return r.content if 200 == r.status_code else 0
    
class HATester:
    def __init__(self, log_dir, host):
        self.__log_dir = log_dir
        self.__host = host
        self.__dbtb = 'hustdbhatb'
        self.__dbkey = 'hustdbhakey'
        self.__dbval = 'hustdbhavalue'
        self.__host = host
        self.__gencmd = lambda cmd: lambda key: '%s/%s?key=%s' % (host, cmd, key)
        self.__gentbcmd = lambda cmd: lambda tb, key: '%s/%s?tb=%s&key=%s' % (host, cmd, tb, key)
        self.__genscmd = lambda cmd: lambda tb: '%s/%s?tb=%s' % (host, cmd, tb)
        self.dict = {
            'stat_all': self.__stat_all,
            'sync_status': self.__sync_status,
            'get_table': self.__get_table,
            'put': self.__put,
            'get': self.__get,
            'get2': self.__get2,
            'del': self.__del,
            'exist': self.__exist,
            
            'hset': self.__hset,
            'hget': self.__hget,
            'hget2': self.__hget2,
            'hdel': self.__hdel,
            'hexist': self.__hexist,
            
            'sadd': self.__sadd,
            'srem': self.__srem,
            'sismember': self.__sismember,
            
            'zadd': self.__zadd,
            'zrem': self.__zrem,
            'zismember': self.__zismember,
            'zscore': self.__zscore,
            'zscore2': self.__zscore2,
            
            'loop': self.__loop
            }
        self.__func_dict = self.__functors()
        self.__count = 0
        self.__sess = requests.Session()
    def __stat_all(self):
        peers = get_count(self.__sess.get, self.__host, 'peer_count')
        for peer in xrange(peers):
            print '[peer-%d]' % peer
            print stat_all(self.__sess.get, self.__host, peer)
        
    def __sync_status(self):
        cmd = '%s/sync_status' % (self.__host)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        print r.content if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
    def __get_table(self):
        cmd = '%s/get_table' % (self.__host)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        print r.content if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
    def __put(self):
        cmd = self.__gencmd('put')(self.__dbkey)
        r = self.__sess.put(cmd, self.__dbval, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        if 200 == r.status_code:
            if 'sync' in r.headers:
                print 'sync: %s' % r.headers['sync']
            else:
                print 'pass'
        else:
            print '%s: %d' % (cmd, r.status_code)
    def __get(self):
        cmd = self.__gencmd('get')(self.__dbkey)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        print r.content if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
    def __get2_complete(self, cmd, r):
        if 'version' in r.headers:
            print 'Version: %s' % r.headers['version']
        elif 'version1' in r.headers:
            print 'Version1: %s' % r.headers['version1']
            print 'Version2: %s' % r.headers['version2']
        if 200 == r.status_code:
            print r.content
        elif 409 == r.status_code:
            if 'val-offset' in r.headers:
                off = int(r.headers['val-offset'])
                print 'Val-Offset: %d' % off
                print 'Value1: %s' % r.content[:off]
                print 'Value2: %s' % r.content[off:]
        else:
            print '%s: %d' % (cmd, r.status_code)
    def __get2(self):
        cmd = self.__gencmd('get2')(self.__dbkey)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        self.__get2_complete(cmd, r)
    def __del(self):
        cmd = self.__gencmd('del')(self.__dbkey)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        if 200 == r.status_code:
            if 'sync' in r.headers:
                print 'sync: %s' % r.headers['sync']
            else:
                print 'pass'
        else:
            print '%s: %d' % (cmd, r.status_code)
    def __exist(self):
        cmd = self.__gencmd('exist')(self.__dbkey)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        print 'exist' if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)

    def __hset(self):
        cmd = self.__gentbcmd('hset')(self.__dbtb, self.__dbkey)
        r = self.__sess.put(cmd, self.__dbval, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        if 200 == r.status_code:
            if 'sync' in r.headers:
                print 'sync: %s' % r.headers['sync']
            else:
                print 'pass'
        else:
            print '%s: %d' % (cmd, r.status_code)
    def __hget(self):
        cmd = self.__gentbcmd('hget')(self.__dbtb, self.__dbkey)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        print r.content if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
    def __hget2(self):
        cmd = self.__gentbcmd('hget2')(self.__dbtb, self.__dbkey)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        self.__get2_complete(cmd, r)
    def __hdel(self):
        cmd = self.__gentbcmd('hdel')(self.__dbtb, self.__dbkey)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        if 200 == r.status_code:
            if 'sync' in r.headers:
                print 'sync: %s' % r.headers['sync']
            else:
                print 'pass'
        else:
            print '%s: %d' % (cmd, r.status_code)
    def __hexist(self):
        cmd = self.__gentbcmd('hexist')(self.__dbtb, self.__dbkey)
        r = self.__sess.get(cmd, auth=(USER, PASSWD))
        print 'exist' if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
        
    def __post_base(self, cmd, body):
        r = self.__sess.post(cmd, body, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        if 200 == r.status_code:
            if 'sync' in r.headers:
                print 'sync: %s' % r.headers['sync']
            else:
                print 'pass'
        else:
            print '%s: %d' % (cmd, r.status_code)
        
    def __sadd(self):
        self.__post_base(self.__genscmd('sadd')(self.__dbtb), self.__dbkey)
    def __srem(self):
        self.__post_base(self.__genscmd('srem')(self.__dbtb), self.__dbkey)
    def __sismember(self):
        cmd = self.__genscmd('sismember')(self.__dbtb)
        r = self.__sess.post(cmd, self.__dbkey, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        print 'pass' if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)

    def __zadd(self):
        self.__post_base('%s/zadd?tb=hustdbhaztb&score=60' % self.__host, self.__dbkey)
    def __zrem(self):
        self.__post_base('%s/zrem?tb=hustdbhaztb' % self.__host, self.__dbkey)
    def __zismember(self):
        cmd = '%s/zismember?tb=hustdbhaztb' % self.__host
        r = self.__sess.post(cmd, self.__dbkey, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        print 'pass' if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
    def __zscore(self):
        cmd = '%s/zscore?tb=hustdbhaztb' % self.__host
        r = self.__sess.post(cmd, self.__dbkey, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        print r.content if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
    def __zscore2(self):
        cmd = '%s/zscore2?tb=hustdbhaztb' % self.__host
        r = self.__sess.post(cmd, self.__dbkey, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        self.__get2_complete(cmd, r)

    def __run_template(self, loop, cmd, func):
        try:
            self.__count = self.__count + 1
            func(loop, cmd)
        except requests.exceptions.RequestException as e:
            log_err(self.__log_dir, 'loop %s {%s: %s}' % (loop, cmd, str(e)))
            sleep(1)
    def __functors(self):
        def __put(val, func):
            def __put_imp(loop, cmd):
                r = func(cmd, val, headers = {'content-type':'text/plain'}, timeout=TIMEOUT, auth=(USER, PASSWD))
                if 200 != r.status_code:
                    log_err(self.__log_dir, 'loop %s { %s: %d }' % (loop, cmd, r.status_code))
                else:
                    if 'sync' in r.headers:
                        log_err(self.__log_dir, 'loop %s { %s: { sync: %s } }' % (loop, cmd, r.headers['sync']))
            return __put_imp
        def __get(val, func):
            def __get_imp(loop, cmd):
                r = func(cmd, timeout=TIMEOUT, auth=(USER, PASSWD))
                if r.content != val:
                    log_err(self.__log_dir, 'loop %s { %s }, not equal' % (loop, cmd))
            return __get_imp
        def __del(func):
            def __del_imp(loop, cmd):
                r = func(cmd, timeout=TIMEOUT, auth=(USER, PASSWD))
                if 200 != r.status_code:
                    log_err(self.__log_dir, 'loop %s { %s: %d }' % (loop, cmd, r.status_code))
                else:
                    if 'sync' in r.headers:
                        log_err(self.__log_dir, 'loop %s { %s: { sync: %s } }' % (loop, cmd, r.headers['sync']))
            return __del_imp
        def __exist(func): 
            def __exist_imp(loop, cmd):
                r = func(cmd, timeout=TIMEOUT, auth=(USER, PASSWD))
                if 200 == r.status_code:
                    log_err(self.__log_dir, 'loop %s { %s: still exist }' % (loop, cmd))
            return __exist_imp
        def __post(key, func):
            def __post_imp(loop, cmd):
                r = func(cmd, key, headers = {'content-type':'text/plain'}, timeout=TIMEOUT, auth=(USER, PASSWD))
                if 200 != r.status_code:
                    log_err(self.__log_dir, 'loop %s { %s: %d }' % (loop, cmd, r.status_code))
                else:
                    if 'sync' in r.headers:
                        log_err(self.__log_dir, 'loop %s { %s: { sync: %s } }' % (loop, cmd, r.headers['sync']))
            return __post_imp
        def __post_exist(key, func):
            def __post_exist_imp(loop, cmd):
                r = func(cmd, key, headers = {'content-type':'text/plain'}, timeout=TIMEOUT, auth=(USER, PASSWD))
                if 200 == r.status_code:
                    log_err(self.__log_dir, 'loop %s { %s: still exist }' % (loop, cmd))
            return __post_exist_imp
        def __sadd(key, func):
            return __post(key, func)
        def __srem(key, func):
            return __post(key, func)
        def __sismember(key, func):
            return __post_exist(key, func)
        def __zadd(key, func):
            return __post(key, func)
        def __zscore(key, score, func):
            def __zscore_imp(loop, cmd):
                r = func(cmd, key, headers = {'content-type':'text/plain'}, timeout=TIMEOUT, auth=(USER, PASSWD))
                if 200 != r.status_code or int(r.content) != score:
                    log_err(self.__log_dir, 'loop %s { %s: score error }' % (loop, cmd))
            return __zscore_imp
        def __zrem(key, func):
            return __post(key, func)
        def __zismember(key, func):
            return __post_exist(key, func)
        return {
            'put': __put,
            'get': __get,
            'del': __del,
            'exist': __exist,
            'sadd': __sadd,
            'srem': __srem,
            'sismember': __sismember,
            'zadd': __zadd,
            'zscore': __zscore,
            'zrem': __zrem,
            'zismember': __zismember
            }
    def __run_each_case(self, loop, tb, key, val):
        cases = [
            ['put', self.__func_dict['put'](val, self.__sess.put)],
            ['get', self.__func_dict['get'](val, self.__sess.get)],
            ['del', self.__func_dict['del'](self.__sess.get)],
            ['exist', self.__func_dict['exist'](self.__sess.get)]
            ]
        hcases = [
            ['hset', self.__func_dict['put'](val, self.__sess.put)],
            ['hget', self.__func_dict['get'](val, self.__sess.get)],
            ['hdel', self.__func_dict['del'](self.__sess.get)],
            ['hexist', self.__func_dict['exist'](self.__sess.get)]
            ]
        scases = [
            ['sadd', self.__func_dict['sadd'](key, self.__sess.post)],
            ['srem', self.__func_dict['srem'](key, self.__sess.post)],
            ['sismember', self.__func_dict['sismember'](key, self.__sess.post)]
            ]
        zcases = [
            ['zadd',      '%s/zadd?tb=hustdbhaztb&score=60' % self.__host, self.__func_dict['zadd'](key, self.__sess.post)],
            ['zscore',    '%s/zscore?tb=hustdbhaztb' % self.__host,        self.__func_dict['zscore'](key, 60, self.__sess.post)],
            ['zrem',      '%s/zrem?tb=hustdbhaztb' % self.__host,          self.__func_dict['zrem'](key, self.__sess.post)],
            ['zismember', '%s/zismember?tb=hustdbhaztb' % self.__host,     self.__func_dict['zismember'](key, self.__sess.post)]
        ]
        for case in cases:
            self.__run_template(loop, self.__gencmd(case[0])(key),       case[1])
        for case in hcases:
            self.__run_template(loop, self.__gentbcmd(case[0])(tb, key), case[1])
        for case in scases:
            self.__run_template(loop, self.__genscmd(case[0])(tb),       case[1])
        for case in zcases:
            self.__run_template(loop, case[1], case[2])
    def __run_cases(self, loop):
        self.__count = 0
        for case in test_cases:
            self.__run_each_case(loop, case['tb'], case['key'], case['val'])
        #print self.__count
    def __loop(self):
        for i in xrange(LOOPS):
            loopstr = str(i)
            print 'loop %s' % loopstr
            with open('%s.count' % self.__log_dir, 'w') as f:
                f.writelines('%s%s\n' % (gentm(), loopstr))
            self.__run_cases(loopstr)

def test(argv):
    log_dir = os.path.join(os.path.abspath('.'), 'hustdb_ha.log')
    size = len(argv)
    if 3 != size:
        return False
    host = 'http://%s' % argv[1]
    cmd = argv[2]
    obj = HATester(log_dir, host)
    if cmd in obj.dict:
        obj.dict[cmd]()
        return True
    return False

if __name__ == "__main__":
    if not test(sys.argv):
        manual()