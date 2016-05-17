#/usr/bin/python
import sys
import os
import time
import datetime
import json
import string
import requests
import random
import multiprocessing
import multiprocessing.dummy

LOOPS = 10000
USER = 'huststore'
PASSWD = 'huststore'
gentm = lambda: datetime.datetime.now().strftime('[%Y-%m-%d %H:%M:%S] ')
test_cases = map(lambda i: { 'tb': 'hustdbhatb%d' % i, 'key': 'hustdbhakey%d' % i, 'val': 'hustdbhaval%d' % i }, xrange(1, 31))
gen_char = lambda i: str('\n' if 0 != i and 0 == i % 80 else random.randint(0, 9))
gen_body = lambda n: map(gen_char, xrange(n))

def manual(): 
    print """
    usage:
        python mutitest [number] [uri]
            number : number of process
            uri    : ip:port
    sample:
        python mutitest.py 2 localhost:8082
        """
    
def log_err(str):
    print str
    with open('hustdb_ha.log', 'a+') as f:
        f.writelines('%s%s\n' % (gentm(), str))

def write_value(loop, sess, cmd, body):
    r = sess.put(cmd, body, headers = {'content-type':'text/plain'}, timeout=5, auth=(USER, PASSWD))
    if 200 != r.status_code:
        log_err('loop %s { %s: %d }' % (loop, cmd, r.status_code))
    else:
        if 'sync' in r.headers:
            log_err('loop %s { %s: { sync: %s } }' % (loop, cmd, r.headers['sync']))

def write_vals(loop, sess, gen_cmd, host, body, blen):
    for case in test_cases:
        index = random.randint(0, blen - 1)
        body[index] = gen_char(index)
        val = ''.join(body)
        cmd = gen_cmd(host, case)
        try:
            write_value(loop, sess, cmd, val)
        except requests.exceptions.RequestException as e:
            log_err('loop %s {%s: %s}' % (loop, cmd, str(e)))
            time.sleep(1)

def post_value(loop, sess, cmd, body):
    r = sess.post(cmd, body, headers = {'content-type':'text/plain'}, timeout=5, auth=(USER, PASSWD))
    if 200 == r.status_code:
        if 'sync' in r.headers:
            print 'sync: %s' % r.headers['sync']
    else:
        print '%s: %d' % (cmd, r.status_code)
def post_values(loop, sess, host, body, blen):
    def __post_value(loop, sess, cmd, key):
        try:
            post_value(loop, sess, cmd, key)
        except requests.exceptions.RequestException as e:
            log_err('loop %s {%s: %s}' % (loop, cmd, str(e)))
            time.sleep(1)
    for case in test_cases:
        index = random.randint(0, blen - 1)
        body[index] = gen_char(index)
        key = ''.join(body)
        cmd = '%s/sadd?tb=%s' % (host, case['tb'])
        __post_value(loop, sess, cmd, key)
        cmd = '%s/zadd?tb=%s&score=60' % (host, case['tb'])
        __post_value(loop, sess, cmd, key)
        
    
def write_values(loop, sess, host, body, blen):
    write_vals(loop, sess, lambda host, case: '%s/put?key=%s' % (host, case['key']), host, body, blen)
    write_vals(loop, sess, lambda host, case: '%s/hset?tb=%s&key=%s' % (host, case['tb'], case['key']), host, body, blen)
    post_values(loop, sess, host, body, blen)

def gloop(arg, **kwarg):
    return MutiLoop.loop(*arg, **kwarg)

class MutiLoop:
    def __init__(self, number, host, blen, body):
        self.__host = host
        self.__blen = blen
        self.__body = body
        self.__number = number
    def loop(self, num):
        sess = requests.Session()
        for i in xrange(LOOPS):
            loopstr = str(i)
            print 'loop %s' % loopstr
            with open('hustdb_ha_loop.log', 'w') as f:
                f.writelines('%s%s\n' % (gentm(), loopstr))
            write_values(loopstr, sess, self.__host, self.__body, self.__blen)
    def mutiprocess(self):
        pool = multiprocessing.Pool(processes=self.__number)
        li = range(self.__number)
        pool.map(gloop, zip([self]*len(li), li))
        pool.close()
        pool.join()
    
def test(argv):
    size = len(argv)
    if 3 != size:
        return False
    number = int(argv[1])
    host = 'http://%s' % argv[2]
    print 'generate string...'
    blen = 1024 * 1024
    body = gen_body(blen)
    obj = MutiLoop(number, host, blen, body)
    obj.mutiprocess()
    return True
                
if __name__ == "__main__":
    if not test(sys.argv):
        manual()