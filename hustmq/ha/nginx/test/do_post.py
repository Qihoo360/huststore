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
USER = 'huststore'
PASSWD = 'huststore'

fib = lambda n:1 if n<=2 else fib(n-1)+fib(n-2)

def do_post(host):
    sess = requests.session()
    cmd = '%s/do_post' % host
    r = sess.post(cmd, auth=(USER, PASSWD))
    if 200 != r.status_code:
        print 'do_post fail, code: %d' % r.status_code
        return
    queue = r.headers['queue']
    token = r.headers['token']
    body = r.content
    print 'token: ', token
    print 'queue: ', queue
    print 'body: ', body
    print 'fib...'
    result = fib(int(body))
    print 'send result: ', result
    cmd = '%s/do_post?token=%s' % (host, token)
    r = sess.post(cmd, str(result), headers={'content-type':'text/plain'}, auth=(USER, PASSWD))
    print 'ok' if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)

def loop_do_post(host):
    sess = requests.session()
    for i in xrange(LOOPS):
        if 0 == i % 100:
            print 'loop ', i
        cmd = '%s/do_post' % host
        r = sess.post(cmd, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print 'do_post fail, code: %d' % r.status_code
            continue
        queue = r.headers['queue']
        token = r.headers['token']
        body = r.content
        result = fib(int(body))
        cmd = '%s/do_post?token=%s' % (host, token)
        r = sess.post(cmd, str(result), headers={'content-type':'text/plain'}, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)

def test(argv):
    size = len(argv)
    if 1 == size:
        do_post('http://localhost:8080')
        return
    host = 'http://localhost:8080' if 2 == size else 'http://%s' % argv[1]
    if 2 == size:
        if 'loop' == argv[1]:
            loop_do_post(host)
    elif 3 == size:
        if 'loop' == argv[2]:
            loop_do_post(host)
    return True
                
if __name__ == "__main__":
    test(sys.argv)