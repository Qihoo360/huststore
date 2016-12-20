#/usr/bin/python
import sys
import os
import datetime
import requests
import random
from time import sleep

LOOPS = 10000
USER = 'huststore'
PASSWD = 'huststore'
gentm = lambda: datetime.datetime.now().strftime('[%Y-%m-%d %H:%M:%S] ')
TIMEOUT = 60

kv_fmt = '%s/%s?key=%s'
hash_fmt = '%s/%s?tb=%s&key=%s'
set_fmt = '%s/%s?tb=%s'

def manual(): 
    print """
    usage:
        python autotest [uri] [action]
            [uri]
                uri: ip:port
            [action]
                put | get | get2 | del | exist |
                hset | hincrby | hget | hget2 |hdel | hexist |
                sadd | srem | sismember | sismember2 |
                zadd | zrem | zismember | zscore | zscore2 |
                cache_exist | cache_get | cache_ttl | 
                cache_put | cache_append | cache_del | cache_expire |
                cache_hexist | cache_hget | cache_hset | cache_hdel |
                cache_hincrby | cache_hincrbyfloat |
                stat_all | sync_status | sync_alive | get_table | loop
    sample:
        python autotest.py localhost:8082 loop
        python autotest.py localhost:8082 stat_all
        python autotest.py localhost:8082 sync_status
        """

def test_loop(log_dir, host, sess):
    def log_err(path, msg):
        print msg
        with open(path, 'a+') as f:
            f.writelines('%s%s\n' % (gentm(), msg))
    def write_complete_base(loop, r, cmd, key):
        if 200 != r.status_code:
            log_err(log_dir, 'loop %s { %s: %d }' % (loop, cmd, r.status_code))
        else:
            if key in r.headers:
                log_err(log_dir, 'loop %s { %s: { %s: %s } }' % (loop, cmd, key, r.headers[key]))
    def write_complete(loop, r, cmd):
        write_complete_base(loop, r, cmd, 'sync')
    def write_cache_complete(loop, r, cmd):
        write_complete_base(loop, r, cmd, 'fail')
    def exist_complete(loop, r, cmd):
        if 200 == r.status_code:
            log_err(log_dir, 'loop %s { %s: still exist }' % (loop, cmd))
    def get_complete(loop, r, cmd, val):
        if r.content != val:
            log_err(log_dir, 'loop %s { %s }, not equal' % (loop, cmd))
    def zscore_complete(loop, r, cmd, score):
        if 200 != r.status_code or int(r.content) != score:
            log_err(log_dir, 'loop %s { %s: score error }' % (loop, cmd))
    def fetch(func, cmd, body):
        return func(cmd, body, headers = {'content-type':'text/plain'}, timeout=TIMEOUT, auth=(USER, PASSWD))
    def fetch_nobody(func, cmd):
        return func(cmd, timeout=TIMEOUT, auth=(USER, PASSWD))
    
    def safe_request(loop, func, cmd, count):
        try:
            func(cmd)
        except requests.exceptions.RequestException as e:
            log_err(log_dir, 'loop %s {%s: %s}' % (loop, cmd, str(e)))
            sleep(1)
        return count + 1
    def run_each_case(loop, sess, hash_tb, set_tb, zset_tb, key, val, score, count):
        kv_cmd = lambda cmd: kv_fmt % (host, cmd, key)
        hash_cmd = (lambda host, tb, key: lambda cmd: hash_fmt % (host, cmd, tb, key))(host, hash_tb, key)
        set_cmd = (lambda host, tb: lambda cmd: set_fmt % (host, cmd, tb))(host, set_tb)
        zset_cmd = (lambda host, tb: lambda cmd: set_fmt % (host, cmd, tb))(host, zset_tb)
        
        put_base = lambda loop, func, val: lambda cmd: write_complete(loop, fetch(func, cmd, val), cmd)
        get_base = lambda loop, func, val: lambda cmd: get_complete(loop, fetch_nobody(func, cmd), cmd, val)
        del_base = lambda loop, func: lambda cmd: write_complete(loop, fetch_nobody(func, cmd), cmd)
        exist_base = lambda loop, func: lambda cmd: exist_complete(loop, fetch_nobody(func, cmd), cmd)
        
        put_key = lambda loop, func, key: lambda cmd: write_complete(loop, fetch(func, cmd, key), cmd)
        rem_key = lambda loop, func, key: lambda cmd: write_complete(loop, fetch(func, cmd, key), cmd)
        key_exist = lambda loop, func, key: lambda cmd: exist_complete(loop, fetch(func, cmd, key), cmd)
        zscore = lambda loop, func, key, score: lambda cmd: zscore_complete(loop, fetch(func, cmd, key), cmd, score)
        
        cases = [
            [kv_cmd('put'), put_base(loop, sess.post, val)],
            [kv_cmd('get'), get_base(loop, sess.get, val)],
            [kv_cmd('del'), del_base(loop, sess.get)],
            [kv_cmd('exist'), exist_base(loop, sess.get)],
            [hash_cmd('hset'), put_base(loop, sess.post, val)],
            [hash_cmd('hget'), get_base(loop, sess.get, val)],
            [hash_cmd('hdel'), del_base(loop, sess.get)],
            [hash_cmd('hexist'), exist_base(loop, sess.get)],
            [set_cmd('sadd'), put_key(loop, sess.post, key)],
            [set_cmd('srem'), rem_key(loop, sess.post, key)],
            [set_cmd('sismember'), key_exist(loop, sess.post, key)],
            ['%s&score=%d' % (zset_cmd('zadd'), score), put_key(loop, sess.post, key)],
            [zset_cmd('zscore'), zscore(loop, sess.post, key, score)],
            [zset_cmd('zrem'), rem_key(loop, sess.post, key)],
            [zset_cmd('zismember'), key_exist(loop, sess.post, key)],
            [kv_cmd('cache/put'), put_base(loop, sess.post, val)],
            [kv_cmd('cache/get'), get_base(loop, sess.get, val)],
            [kv_cmd('cache/del'), del_base(loop, sess.get)],
            [kv_cmd('cache/exist'), exist_base(loop, sess.get)],
            [hash_cmd('cache/hset'), put_base(loop, sess.post, val)],
            [hash_cmd('cache/hget'), get_base(loop, sess.get, val)],
            [hash_cmd('cache/hdel'), del_base(loop, sess.get)],
            [hash_cmd('cache/hexist'), exist_base(loop, sess.get)]
            ]
        for case in cases:
            safe_request(loop, case[1], case[0], count)
        return count
    def run_cases(loop):
        count = 0
        for i in xrange(1, 31):
            count = run_each_case(loop, sess, 
                'hustdbhahashtbloop', 
                'hustdbhasettbloop', 
                'hustdbhazsettbloop', 
                'hustdbhakey%d' % i, 
                'hustdbhaval%d' % i, 
                i, count)
        #print count
    for i in xrange(LOOPS):
        loopstr = str(i)
        print 'loop %s' % loopstr
        with open('%s.count' % log_dir, 'w') as f:
            f.writelines('%s%s\n' % (gentm(), loopstr))
        run_cases(loopstr)

def test_ha(log_dir, host, cmd):
    # test utils
    def stat_all(sess, host):
        def __get_count(sess, host, key):
            cmd = '%s/%s' % (host, key)
            r = sess.get(cmd, auth=(USER, PASSWD))
            return int(r.content) if 200 == r.status_code else 0
        def __stat_all(sess, host, peer):
            cmd = '%s/stat_all?peer=%d' % (host, peer)
            r = sess.get(cmd, auth=(USER, PASSWD))
            return r.content if 200 == r.status_code else 0
        peers = __get_count(sess, host, 'peer_count')
        for peer in xrange(peers):
            print '[peer-%d]' % peer
            print __stat_all(sess, host, peer)
    def write_complete_base(r, cmd, key):
        if 200 == r.status_code:
            if key in r.headers:
                print '%s: %s' % (key, r.headers[key])
            else:
                print 'pass'
        else:
            print '%s: %d' % (cmd, r.status_code)
    def write_complete(r, cmd):
        write_complete_base(r, cmd, 'sync')
    def write_cache_complete(r, cmd):
        write_complete_base(r, cmd, 'fail')
    def get2_complete(r, cmd):
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
    def exist2_complete(r, cmd):
        if 'version' in r.headers:
            print 'Version: %s' % r.headers['version']
        elif 'version1' in r.headers:
            print 'Version1: %s' % r.headers['version1']
            print 'Version2: %s' % r.headers['version2']
        if 200 == r.status_code:
            print 'ok'
        elif 409 == r.status_code:
            print 'conflict version'
        else:
            print '%s: %d' % (cmd, r.status_code)
    def print_result(r, cmd, result):
        print result if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)
    def http_post(sess, cmd, body):
        return sess.post(cmd, body, headers = {'content-type':'text/plain'}, auth=(USER, PASSWD))
    def post_nobody_base(sess, cmd, body):
        print_result(http_post(sess, cmd, body), cmd, 'pass')
    def post_base(sess, cmd, body):
        r = http_post(sess, cmd, body)
        print_result(r, cmd, r.content)
    def post2_base(sess, cmd, body):
        r = http_post(sess, cmd, body)
        get2_complete(r, cmd)
    def post_cache_base(sess, cmd, body):
        write_cache_complete(http_post(sess, cmd, body), cmd)
    def set_cache_base(sess, cmd):
        write_cache_complete(sess.get(cmd, auth=(USER, PASSWD)), cmd)
    def write_body_base(sess, cmd, body):
        write_complete(http_post(sess, cmd, body), cmd)
    def write_base(sess, cmd):
        write_complete(sess.get(cmd, auth=(USER, PASSWD)), cmd)
    def get2_base(sess, cmd):
        get2_complete(sess.get(cmd, auth=(USER, PASSWD)), cmd)
    def get_base(sess, cmd):
        r = sess.get(cmd, auth=(USER, PASSWD))
        print_result(r, cmd, r.content)
    def get_nobody_base(sess, cmd, result):
        print_result(sess.get(cmd, auth=(USER, PASSWD)), cmd, result)
    def hincrby_base(sess, cmd, val):
        set_cache_base(sess, '%s&val=%s' % (hash_cmd('cache/hset'), val))
        set_cache_base(sess, '%s&val=%s' % (hash_cmd('cache/%s' % cmd), val))
        get_base(sess, hash_cmd('cache/hget'))
    def hincrby_impl(sess, val):
        write_base(sess, '%s&val=%s' % (hash_cmd('hset'), val))
        get_base(sess, '%s&val=1' % hash_cmd('hincrby'))

    # vars
    sess = requests.Session()
    
    kv_cmd = (lambda host, key: lambda cmd: kv_fmt % (host, cmd, key))(host, 'hustdbhakey')
    hash_cmd = (lambda host, tb, key: lambda cmd: hash_fmt % (host, cmd, tb, key))(host, 'hustdbhahashtb', 'hustdbhahashkey')
    set_cmd = (lambda host, tb: lambda cmd: set_fmt % (host, cmd, tb))(host, 'hustdbhasettb')
    zset_cmd = (lambda host, tb: lambda cmd: set_fmt % (host, cmd, tb))(host, 'hustdbhazsettb')
    
    kv_val = 'hustdbhavalue'
    hash_val = 'hustdbhahashvalue'
    
    set_key = 'hustdbhasetkey'
    zset_key = 'hustdbhazsetkey'

    func_dict = {
        'loop': lambda: test_loop(log_dir, host, sess),
        'stat_all': (lambda sess, host: lambda: stat_all(sess, host))(sess, host),
        'sync_status': lambda: get_base(sess, '%s/sync_status' % (host)),
        'sync_alive': lambda: get_base(sess, '%s/sync_alive' % (host)),
        'get_table': lambda: get_base(sess, '%s/get_table' % (host)),
        # kv
        'put': lambda: write_body_base(sess, kv_cmd('put'), kv_val),
        'get': lambda: get_base(sess, kv_cmd('get')),
        'get2': lambda: get2_base(sess, cmd = kv_cmd('get2')),
        'del': lambda: write_base(sess, kv_cmd('del')),
        'exist': lambda: get_nobody_base(sess, kv_cmd('exist'), 'exist'),
        # hash
        'hset': lambda: write_body_base(sess, hash_cmd('hset'), hash_val),
        'hincrby': lambda: hincrby_impl(sess, '5'),
        'hget': lambda: get_base(sess, hash_cmd('hget')),
        'hget2': lambda: get2_base(sess, hash_cmd('hget2')),
        'hdel': lambda: write_base(sess, hash_cmd('hdel')),
        'hexist': lambda: get_nobody_base(sess, hash_cmd('hexist'), 'exist'),
        # set
        'sadd': lambda: write_body_base(sess, set_cmd('sadd'), set_key),
        'srem': lambda: write_body_base(sess, set_cmd('srem'), set_key),
        'sismember': lambda: post_nobody_base(sess, set_cmd('sismember'), set_key),
        'sismember2': (lambda cmd: lambda: exist2_complete(http_post(sess, cmd, set_key), cmd))(set_cmd('sismember2')),
        # zset
        'zadd': lambda: write_body_base(sess, '%s&score=60' % zset_cmd('zadd'), zset_key),
        'zrem': lambda: write_body_base(sess, zset_cmd('zrem'), zset_key),
        'zismember': lambda: post_nobody_base(sess, zset_cmd('zismember'), zset_key),
        'zscore': lambda: post_base(sess, zset_cmd('zscore'), zset_key),
        'zscore2': lambda: post2_base(sess, zset_cmd('zscore2'), zset_key),
        # cache
        'cache_exist': lambda: get_nobody_base(sess, kv_cmd('cache/exist'), 'exist'),
        'cache_get': lambda: get_base(sess, kv_cmd('cache/get')),
        'cache_ttl': lambda: get_base(sess, kv_cmd('cache/ttl')),
        'cache_put': lambda: post_cache_base(sess, kv_cmd('cache/put'), kv_val),
        'cache_append': lambda: post_cache_base(sess, kv_cmd('cache/append'), kv_val),
        'cache_del': lambda: post_cache_base(sess, kv_cmd('cache/del'), ''),
        'cache_expire': lambda: post_cache_base(sess, '%s&ttl=5' % kv_cmd('cache/expire'), ''),
        'cache_hexist': lambda: get_nobody_base(sess, hash_cmd('cache/hexist'), 'exist'),
        'cache_hget': lambda: get_base(sess, hash_cmd('cache/hget')),
        'cache_hset': lambda: post_cache_base(sess, hash_cmd('cache/hset'), hash_val),
        'cache_hdel': lambda: post_cache_base(sess, hash_cmd('cache/hdel'), ''),
        'cache_hincrby': lambda: hincrby_base(sess, 'hincrby', '5'),
        'cache_hincrbyfloat': lambda: hincrby_base(sess, 'hincrbyfloat', '5.125')
        }
    if cmd in func_dict:
        func_dict[cmd]()
        return True
    return False

def test_main(argv):
    log_dir = os.path.join(os.path.abspath('.'), 'hustdb_ha.log')
    size = len(argv)
    if 3 != size:
        return False
    return test_ha(log_dir, host = 'http://%s' % argv[1], cmd = argv[2])

if __name__ == "__main__":
    if not test_main(sys.argv):
        manual()