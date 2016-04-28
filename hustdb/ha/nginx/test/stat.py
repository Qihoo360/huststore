#/usr/bin/python
import sys
import string
import requests

USER = 'huststore'
PASSWD = 'huststore'

def peer_count(func, host):
    cmd = '%s/peer_count' % (host)
    r = func(cmd, auth=(USER, PASSWD))
    return int(r.content) if 200 == r.status_code else 0
def stat(func, host, peer):
    cmd = '%s/stat?peer=%d' % (host, peer)
    r = func(cmd, auth=(USER, PASSWD))
    return r.content if 200 == r.status_code else 'no data'
def stat_tb(func, host, peer, tb):
    cmd = '%s/stat?peer=%d&tb=%s' % (host, peer, tb)
    r = func(cmd, auth=(USER, PASSWD))
    return r.content if 200 == r.status_code else 'no data'
def test_stat(host):
    sess = requests.Session()
    tb = 'test_table'
    print '[stat k-v]'
    for peer in xrange(peer_count(sess.get, host)):
        print stat(sess.get, host, peer)
    print '[stat table "%s"]' % tb
    for peer in xrange(peer_count(sess.get, host)):
        print stat_tb(sess.get, host, peer, tb)
def get_host(argv):
    size = len(argv)
    if 2 == size:
        return 'http://%s:8082' % argv[1]
    return 'http://localhost:8082'
                
if __name__ == "__main__":
    test_stat(get_host(sys.argv))