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

def do_get(host):
    sess = requests.session()
    cmd = '%s/do_get?queue=fibonacci' % host
    body = '10'
    r = sess.post(cmd, body, headers={'content-type':'text/plain'}, auth=(USER, PASSWD))
    print r.content if 200 == r.status_code else '%s: %d' % (cmd, r.status_code)

def loop_do_get(host):
    sess = requests.session()
    for i in xrange(LOOPS):
        if 0 == i % 100:
            print 'loop ', i
        cmd = '%s/do_get?queue=fibonacci' % host
        body = str(i % 10 + 1)
        r = sess.post(cmd, body, headers={'content-type':'text/plain'}, auth=(USER, PASSWD))
        if 200 != r.status_code:
            print '%s: %d' % (cmd, r.status_code)

def test(argv):
    size = len(argv)
    if 1 == size:
        do_get('http://localhost:8080')
        return
    host = 'http://localhost:8080' if 2 == size else 'http://%s' % argv[1]
    if 2 == size:
        if 'loop' == argv[1]:
            loop_do_get(host)
    elif 3 == size:
        if 'loop' == argv[2]:
            loop_do_get(host)
    return True
                
if __name__ == "__main__":
    test(sys.argv)