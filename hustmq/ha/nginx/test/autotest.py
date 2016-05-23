#/usr/bin/python
import sys
import os
import time
import datetime
import json
import string
import requests
import random
from time import sleep

LOOPS = 10000
gentm = lambda: datetime.datetime.now().strftime('[%Y-%m-%d %H:%M:%S] ')
test_queues = map(lambda i: 'hustmqhaqueue%d' % i, xrange(1, 33))
gen_body = lambda n: ''.join(map(lambda i: str('\n' if 0 != i and 0 == i % 80 else random.randint(0, 9)), xrange(n)))
gen_indexs = lambda item: filter(lambda i: item['ready'][i] > 0, range(0, 3))
USER = 'huststore'
PASSWD = 'huststore'

def manual(): 
    print """
    usage:
        python autotest [uri] [action]
            [uri]
                ip:port
            [action]
                loop
                stat_all
                put
                get
                ack
                timeout
                evget
                evput
                evsub
                evpub
                worker
                purge
                do_get_status
                do_post_status
    sample:
        python autotest.py 192.168.1.101:8080 stat_all
        python autotest.py 192.168.1.101:8080 put
        python autotest.py 192.168.1.101:8080 get
        """

def log_err(str):
    print str
    with open('hustmq_ha.log', 'a+') as f:
        f.writelines('%s%s\n' % (gentm(), str))

def log_pub(str):
    print str
    with open('pub_err.log', 'a+') as f:
        f.writelines('%s%s\n' % (gentm(), str))
        
class HATester:
    def __init__(self, host):
        self.__host = host
        self.__gen_put_item = lambda i: {
            'cmd': (lambda i: lambda q: '%s/put?queue=%s&item=hustmqteststr%d' % (self.__host, q, i))(i)
            }
        self.__gen_put_body_item = lambda i: {
            'cmd': lambda q: '%s/put?queue=%s' % (self.__host, q),
            'body': gen_body(1024 + i)
            }
        self.__gen_purge_item = lambda i, q: '%s/purge?queue=%s&priori=%d' % (self.__host, q, i)
        self.dict = {
            'purge': self.__purge,
            'loopst': self.__loopst,
            'stat_all': self.__stat_all,
            'put': self.__put,
            'get': self.__get,
            'ack': self.__ack,
            'timeout': self.__timeout,
            'queue_hash': self.__queue_hash,
            'evget': self.__evget,
            'evput': self.__evput,
            'evsub': self.__evsub,
            'evpub': self.__evpub,
            'pub': self.__pub,
            'looppub': self.__looppub,
            'worker': self.__worker,
            'do_get_status': self.__do_get_status,
            'do_post_status': self.__do_post_status
            }
        self.__blen = 1024 * 1024
        self.__count = 0

    def __do_get_status(self):
        cmd = '%s/do_get_status' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        print r.content if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
    def __do_post_status(self):
        cmd = '%s/do_post_status' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        print r.content if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
    def __purge_item(self, item, index):
        try:
            cmd = self.__gen_purge_item(index, item['queue'])
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
        except requests.exceptions.RequestException as e:
            print 'HATester::__purge_item->error: %s: %s' % (cmd, str(e))
    def __purge(self):
        items = filter(lambda item: item['queue'] in test_queues, self.__get_stat_all())
        for item in items:
            for i in gen_indexs(item):
                self.__purge_item(item, i)
    def __stat_all(self):
        self.__autost()
        cmd = '%s/stat_all' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
        else:
            print '%s: %s' % (cmd, r.content)
    def __autost(self):
        self.__count = self.__count + 1
        cmd = '%s/autost' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 == r.status_code:
            return True
        print '%s: %d' % (cmd, r.status_code)
        return False
    def __loopst(self):
        for i in xrange(LOOPS):
            if i % 100 == 0:
                print 'loop %d' % i
            cmd = '%s/autost' % self.__host
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
    def __put(self):
        cmd = '%s/put?queue=hustmqhaqueue' % self.__host
        r = requests.put(cmd, 'test_body', headers={'content-type':'text/plain'}, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
        else:
            print 'pass'
    def __get(self):
        cmd = '%s/get?queue=hustmqhaqueue&worker=testworker' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
        else:
            print r.content
    def __ack(self):
        cmd = '%s/get?queue=hustmqhaqueue&worker=testworker&ack=false' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
            return
        print r.content
        cmd = '%s/ack?queue=hustmqhaqueue&peer=%s&token=%s' % (
            self.__host, r.headers['ack-peer'], r.headers['ack-token'])
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
            return
        print 'acked'
    def __timeout(self):
        cmd = '%s/timeout?queue=hustmqhaqueue&minute=1' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
        else:
            print 'ok'
    def __queue_hash(self):
        get_body = lambda i: 'test_body_%d' % i
        print 'put...'
        for i in xrange(100):
            cmd = '%s/put?queue=hustmqhaqueue' % self.__host
            r = requests.put(cmd, get_body(i), headers={'content-type':'text/plain'}, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
                return
        print 'get...'
        for i in xrange(100):
            cmd = '%s/get?queue=hustmqhaqueue&worker=testworker' % self.__host
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
                return
            body = get_body(i)
            if r.content != body:
                print 'bad body: %s' % (cmd)
                return
        print 'pass'
    def __evput(self):
        for i in xrange(LOOPS):
            if i % 100 == 0:
                print 'loop %d' % i
            cmd = '%s/put?queue=hustmqhaqueue&item=hustmqteststr' % self.__host
            r = requests.put(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
            cmd = '%s/autost' % self.__host
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
    def __evget(self):
        for i in xrange(LOOPS):
            if i % 100 == 0:
                print 'loop %d' % i
            cmd = '%s/evget?queue=hustmqhaqueue&worker=testworker' % self.__host
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
    def __pub_imp(self):
        cmd = '%s/pub?queue=hustpushqueue&item=jobstest' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
        cmd = '%s/autost' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
    def __evpub(self):
        for i in xrange(LOOPS):
            if i % 100 == 0:
                print 'loop %d' % i
            self.__pub_imp()
        cmd = '%s/autost' % self.__host
        requests.get(cmd, auth=(USER, PASSWD))
    def __pub(self):
        cmd = '%s/pub?queue=hustpushqueue&item=jobstest' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            log_pub('[publish] %s: %d' % (cmd, r.status_code))
        else: 
            print r.headers
    def __looppub(self):
        def __get_idx():
            cmd = '%s/stat?queue=hustpushqueue' % self.__host
            r = requests.get(cmd, auth=(USER, PASSWD))
            if len(r.content) < 1:
                return None
            si = r.json()['si']
            ci = r.json()['ci']
            return { 'si': si,  'ci': ci }
        def __reset():
            idx = __get_idx()
            if not idx:
                return
            cmd = '%s/pub?queue=hustpushqueue&item=jobstest&idx=%d' % (self.__host, int(idx['ci']) + 2)
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                log_pub('[reset idx fail] %s: %d' % (cmd, r.status_code))
            else:
                self.__autost()
                idx = __get_idx()
                log_pub('[reset idx success] %s' % str(idx))
        __reset()
        idx = __get_idx()
        tm = int(time.time())
        err_count = 0
        partial = 0
        for i in xrange(LOOPS):
            if i % 100 == 0:
                print 'loop %d' % i
                new_idx = __get_idx()
                if not idx:
                    idx = new_idx
                new_tm = int(time.time())
                delta = new_tm - tm
                if (new_idx and idx) and (new_idx['si'] != idx['si']):
                    err_count = err_count + 1
                    log_pub('[si changed] old: {si: %d, ci: %d}, new: {si: %d, ci: %d}, interval: %d' % (
                        idx['si'], idx['ci'], new_idx['si'], new_idx['ci'], delta))
                    tm = new_tm
                    idx = new_idx
            cmd = '%s/pub?queue=hustpushqueue&item=jobstest' % self.__host
            try:
                r = requests.get(cmd, auth=(USER, PASSWD))
                if 200 != r.status_code:
                    log_pub('[publish] %s: %d' % (cmd, r.status_code))
                else:
                    if 'count' in r.headers:
                        partial = partial + 1
                        log_pub('[publish] count: %s' % r.headers['count'])
            except requests.exceptions.RequestException as e:
                log_pub('[publish] requests.exceptions.RequestException')
                sleep(0.2)
        log_pub('[publish] total error: %d, partial failure: %d' % (err_count, partial))
        
    def __sub_imp(self, idx):
        while True:
            cmd = '%s/evsub?queue=hustpushqueue&idx=%d' % (self.__host, idx)
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 403 == r.status_code:
                idx = int(r.headers['index'].split('-')[0]) + 1
                print '[sync] %s { si: %d }' % (cmd, idx)
                continue
            elif 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
            break
        return idx + 1
    def __evsub(self):
        idx = 1
        for i in xrange(LOOPS):
            if i % 100 == 0:
                print 'loop %d' % i
            idx = self.__sub_imp(idx)
            
    def __worker(self):
        print 'put...'
        for i in xrange(64):
            cmd = '%s/put?queue=hustmqhaqueue&item=hustmqteststr%d' % (self.__host, i)
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
                return
        self.__autost()
        print 'get...'
        for i in xrange(63):
            cmd = '%s/get?queue=hustmqhaqueue&worker=testworker%d' % (self.__host, i)
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                print '%s: %d' % (cmd, r.status_code)
                return
        self.__autost()
        cmd = '%s/worker?queue=hustmqhaqueue' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
            return
        print r.content
        self.__autost()
        print 'purge...'
        cmd = '%s/stat?queue=hustmqhaqueue' % self.__host
        r = requests.get(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)
            return
        item = r.json()
        for i in gen_indexs(item):
            self.__purge_item(item, i)
    
    def __get_stat_all(self):
        self.__autost()
        r = requests.get('%s/stat_all' % self.__host, auth=(USER, PASSWD))
        return r.json() if 200 == r.status_code else []
    def __run_put_cases(self, number, loop):
        test_cases = map(self.__gen_put_item, xrange(0, number)) + map(self.__gen_put_body_item, xrange(0, number))
        for queue in test_queues:
            for case in test_cases:
                cmd = case['cmd'](queue)
                try:
                    self.__count = self.__count + 1
                    r = requests.put(
                        cmd, data=case['body'], headers={'content-type':'text/plain'}, timeout=5, auth=(USER, PASSWD)
                        ) if 'body' in case else requests.get(cmd, timeout=5, auth=(USER, PASSWD))
                    if 200 != r.status_code:
                        log_err('loop %s HATester::__run_put_cases(%d)->error: %s: %d' % (loop, number, cmd, r.status_code))
                except requests.exceptions.RequestException as e:
                    log_err('loop %s HATester::__run_put_cases(%d)->error: %s: %s' % (loop, number, cmd, str(e)))
                    sleep(1)
    def __run_common_cases(self, loop):
        cases = [
            lambda q: '%s/get?queue=%s&worker=testworker' % (self.__host, q),
            lambda q: '%s/stat?queue=%s' % (self.__host, q),
            lambda q: '%s/max?queue=%s&num=1024' % (self.__host, q),
            lambda q: '%s/lock?queue=%s&on=1' % (self.__host, q),
            lambda q: '%s/lock?queue=%s&on=0' % (self.__host, q)
            ]
        items = filter(lambda item: item['queue'] in test_queues, self.__get_stat_all())
        for item in items:
            for case in cases:
                cmd = case(item['queue'])
                try:
                    self.__count = self.__count + 1
                    r = requests.get(cmd, timeout=5, auth=(USER, PASSWD))
                    if 200 != r.status_code:
                        log_err('loop %s HATester::__run_common_cases->error: %s: %d' % (loop, cmd, r.status_code))
                except requests.exceptions.RequestException as e:
                    log_err('loop %s HATester::__run_common_cases->error: %s: %s' % (loop, cmd, str(e)))
                    sleep(1)
    def __run_worker_cases(self, loop):
        cases = [
            lambda q, l: '%s/put?queue=%s&item=hustmqteststr%s' % (self.__host, q, l),
            lambda q, l: '%s/get?queue=%s&worker=testworker%s' % (self.__host, q, l),
            lambda q, l: '%s/worker?queue=%s' % (self.__host, q),
            ]
        items = filter(lambda item: item['queue'] in test_queues, self.__get_stat_all())
        for item in items:
            r = None
            for case in cases:
                cmd = case(item['queue'], loop)
                try:
                    self.__count = self.__count + 1
                    r = requests.get(cmd, timeout=5, auth=(USER, PASSWD))
                    if 200 != r.status_code:
                        log_err('loop %s HATester::__run_worker_cases->error: %s: %d' % (loop, cmd, r.status_code))
                except requests.exceptions.RequestException as e:
                    log_err('loop %s HATester::__run_worker_cases->error: %s: %s' % (loop, cmd, str(e)))
                    sleep(1)
            if len(filter(lambda o: o['w'] == 'testworker%s' % loop, r.json())) < 1:
                log_err('loop %s HATester::__run_worker_cases->incorrect worker: %s' % (loop, r.content))
    def __run_purge_case(self, item, index, loop):
        try:
            self.__count = self.__count + 1
            cmd = self.__gen_purge_item(index, item['queue'])
            r = requests.get(cmd, auth=(USER, PASSWD))
            if 200 != r.status_code:
                log_err('loop %s HATester::__run_purge_case->error: %s: %d' % (loop, cmd, r.status_code))
        except requests.exceptions.RequestException as e:
            log_err('loop %s HATester::__run_purge_case->error: %s: %s' % (loop, cmd, str(e)))
            sleep(1)
    def __run_purge_cases(self, loop):
        items = filter(lambda item: item['queue'] in test_queues, self.__get_stat_all())
        for item in items:
            for i in gen_indexs(item):
                self.__run_purge_case(item, i, loop)
    def run_cases(self, number, loop):
        self.__count = 0
        self.__autost()
        self.__run_put_cases(2, loop)
        self.__autost()
        self.__run_worker_cases(loop)
        self.__autost()
        self.__run_common_cases(loop)
        self.__autost()
        self.__run_put_cases(number, loop)
        self.__autost()
        self.__run_purge_cases(loop)
        #print self.__count
    def run_put_case(self):
        self.__run_put_cases(1, 0)
    
def loop(obj):
    for i in xrange(LOOPS):
        loopstr = str(i)
        print 'loop %s' % loopstr
        with open('hustmq_ha_loop.log', 'w') as f:
            f.writelines('%s%s\n' % (gentm(), loopstr))
        obj.run_cases(1, loopstr)

def test(argv):
    size = len(argv)
    if 3 != size:
        return False
    cmd = argv[2]
    size = len(argv)
    obj = HATester('http://%s' % argv[1])
    if cmd in obj.dict:
        obj.dict[cmd]()
    elif 'loop' == cmd:
        loop(obj)
    return True
                
if __name__ == "__main__":
    if not test(sys.argv):
        manual()