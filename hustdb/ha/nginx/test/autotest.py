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
        python autotest [option] [action]
            [option]
                host: ip or domain
            [action]
                stat_all | sync_status | get_table | export
                put | get | del | exist | print | clean
                hset | hget | hdel | hexist
                sadd | srem | sismember
                loop | bloop | loopput | loopbput | loopstatus
    sample:
        python autotest.py loop
        python autotest.py stat_all
        python autotest.py sync_status
        python autotest.py print
        python autotest.py clean
        python autotest.py 192.168.1.101 loop
        python autotest.py 192.168.1.101 print
        python autotest.py 192.168.1.101 clean
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

def export_keys(func, host):
    def __export_key(func, host, peer, file_count):
        start = 0
        end = 1024
        for file in xrange(file_count):
            cmd = '%s/export?peer=%d&file=%d&start=%d&end=%d' % (
                host, peer, file, start, end)
            r = func(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '[peer-%d][file-%d] %d' % (peer, file, r.status_code)
    peers = get_count(func, host, 'peer_count')
    for peer in xrange(peers):
        print 'export_keys peer-%d' % peer
        file_count = get_count(func, host, 'file_count?peer=%d' % peer)
        __export_key(func, host, peer, file_count)
    
def export_tbkeys(func, host, tb):
    def __export_tbkeys(func, host, peer, tb):
        cmd = '%s/export?peer=%d&tb=%s' % (host, peer, tb)
        r = func(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '[peer-%d][tb-%s] %d' % (peer, tb, r.status_code)
    peers = get_count(func, host, 'peer_count')
    for peer in xrange(peers):
        print 'export_tbkeys peer-%d' % peer
        __export_tbkeys(func, host, peer, tb)
    
class HATester:
    def __init__(self, log_dir, host):
        self.__log_dir = log_dir
        self.__host = host
        self.__blen = 1024 * 1024
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
            'del': self.__del,
            'exist': self.__exist,
            
            'hset': self.__hset,
            'hget': self.__hget,
            'hdel': self.__hdel,
            'hexist': self.__hexist,
            
            'sadd': self.__sadd,
            'srem': self.__srem,
            'sismember': self.__sismember,
            
            'export': self.__export,
            
            'loop': self.__loop,
            'bloop': self.__bloop,
            'loopput': self.__loopput,
            'loopbput': self.__loopbput,
            'loopstatus': self.__loopstatus,
            'print': self.__print,
            'clean': self.__clean
            }
        self.__func_dict = self.__functors()
        self.__count = 0
        self.__sess = requests.Session()
    def __export(self):
        export_keys(self.__sess.get, self.__host)
        export_tbkeys(self.__sess.get, self.__host, self.__dbtb)
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
        
    def __sadd(self):
        cmd = self.__genscmd('sadd')(self.__dbtb)
        r = self.__sess.post(cmd, self.__dbkey, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        if 200 == r.status_code:
            if 'sync' in r.headers:
                print 'sync: %s' % r.headers['sync']
            else:
                print 'pass'
        else:
            print '%s: %d' % (cmd, r.status_code)
    def __srem(self):
        cmd = self.__genscmd('srem')(self.__dbtb)
        r = self.__sess.post(cmd, self.__dbkey, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        if 200 == r.status_code:
            if 'sync' in r.headers:
                print 'sync: %s' % r.headers['sync']
            else:
                print 'pass'
        else:
            print '%s: %d' % (cmd, r.status_code)
    def __sismember(self):
        cmd = self.__genscmd('sismember')(self.__dbtb)
        r = self.__sess.post(cmd, self.__dbkey, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
        print 'pass' if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
        
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
        def __sadd(key, func):
            def __sadd_imp(loop, cmd):
                r = func(cmd, key, headers = {'content-type':'text/plain'}, timeout=TIMEOUT, auth=(USER, PASSWD))
                if 200 != r.status_code:
                    log_err(self.__log_dir, 'loop %s { %s: %d }' % (loop, cmd, r.status_code))
                else:
                    if 'sync' in r.headers:
                        log_err(self.__log_dir, 'loop %s { %s: { sync: %s } }' % (loop, cmd, r.headers['sync']))
            return __sadd_imp
        def __srem(key, func):
            def __srem_imp(loop, cmd):
                r = func(cmd, key, headers = {'content-type':'text/plain'}, timeout=TIMEOUT, auth=(USER, PASSWD))
                if 200 != r.status_code:
                    log_err(self.__log_dir, 'loop %s { %s: %d }' % (loop, cmd, r.status_code))
                else:
                    if 'sync' in r.headers:
                        log_err(self.__log_dir, 'loop %s { %s: { sync: %s } }' % (loop, cmd, r.headers['sync']))
            return __srem_imp
        def __sismember(key, func):
            def __sismember_imp(loop, cmd):
                r = func(cmd, key, headers = {'content-type':'text/plain'}, timeout=TIMEOUT, auth=(USER, PASSWD))
                if 200 == r.status_code:
                    log_err(self.__log_dir, 'loop %s { %s: still exist }' % (loop, cmd))
            return __sismember_imp
        return {
            'put': __put,
            'get': __get,
            'del': __del,
            'exist': __exist,
            'sadd': __sadd,
            'srem': __srem,
            'sismember': __sismember
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
        for case in cases:
            self.__run_template(loop, self.__gencmd(case[0])(key),       case[1])
        for case in hcases:
            self.__run_template(loop, self.__gentbcmd(case[0])(tb, key), case[1])
        for case in scases:
            self.__run_template(loop, self.__genscmd(case[0])(tb),       case[1])
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
    def __run_bcase(self, loop, val):
        self.__count = 0
        for case in test_cases:
            self.__run_each_case(loop, case['tb'], case['key'], val)
        #print self.__count
    def __bloop(self):
        print 'generate string...'
        body = gen_body(self.__blen)
        val = ''.join(body)
        for i in xrange(LOOPS):
            loopstr = str(i)
            print 'loop %s' % loopstr
            with open('hustdb_ha_loop.log', 'w') as f:
                f.writelines('%s%s\n' % (gentm(), loopstr))
            self.__run_bcase(loopstr, val)
    def __print(self):
        for case in test_cases:
            cmd = self.__gencmd('exist')(case['key'])
            r = self.__sess.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                continue
            cmd = self.__gencmd('get')(case['key'])
            r = self.__sess.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                continue
            size = len(r.content)
            print '{ key: %s, version: %s, value: %s }' % (case['key'], r.headers['version'], 'length=%d' % size if size > 64 else r.content)
    def __clean(self):
        for case in test_cases:
            cmd = self.__gencmd('exist')(case['key'])
            r = self.__sess.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                continue
            cmd = self.__gencmd('del')(case['key'])
            r = self.__sess.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                continue
            print '"%s": clean success' % case['key']
    def __put_case(self, loop):
        for case in test_cases:
            self.__run_template(loop, self.__gencmd('put')(case['key']), self.__func_dict['put'](case['val'], self.__sess.put))
    def __loopput(self):
        for i in xrange(LOOPS):
            loopstr = str(i)
            print 'loop %s' % loopstr
            with open('hustdb_ha_loop.log', 'w') as f:
                f.writelines('%s%s\n' % (gentm(), loopstr))
            self.__put_case(loopstr)
    def __bput_case(self, loop, body):
        for case in test_cases:
            index = random.randint(0, self.__blen - 1)
            body[index] = gen_char(index)
            val = ''.join(body)
            self.__run_template(loop, self.__gencmd('put')(case['key']), self.__func_dict['put'](val, self.__sess.put))
    def __loopbput(self):
        print 'generate string...'
        body = gen_body(self.__blen)
        for i in xrange(LOOPS):
            loopstr = str(i)
            print 'loop %s' % loopstr
            with open('hustdb_ha_loop.log', 'w') as f:
                f.writelines('%s%s\n' % (gentm(), loopstr))
            self.__bput_case(loopstr, body)
    def __loopstatus(self):
        for i in xrange(LOOPS):
            loopstr = str(i)
            print 'loop %s' % loopstr
            with open('hustdb_ha_loop.log', 'w') as f:
                f.writelines('%s%s\n' % (gentm(), loopstr))
            cmd = '%s/sync_status' % (self.__host)
            r = self.__sess.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)

def test(argv):
    log_dir = os.path.join(os.path.abspath('.'), 'hustdb_ha.log')
    size = len(argv)
    if size < 2 or size > 3:
        return False
    host = 'http://localhost:8082' if 2 == size else 'http://%s:8082' % argv[1]
    cmd = argv[1] if 2 == size else argv[2]
    
    obj = HATester(log_dir, host)
    if cmd in obj.dict:
        obj.dict[cmd]()
        return True
    return False
                
if __name__ == "__main__":
    if not test(sys.argv):
        manual()