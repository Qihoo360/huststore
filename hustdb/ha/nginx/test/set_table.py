#/usr/bin/python
import sys
import rsa
import json
import requests
import multiprocessing
import multiprocessing.dummy

AUTH = ('huststore', 'huststore')

def manual(): 
    print """
    usage:
        python set_table.py [option] [host] [private pem] [table]
            [option]
                -t: simple test
                -m: mutiprocess test
    sample:
        python set_table.py -t localhost conf/private.pem conf/hustdbtable.json
        """

def get_identifier(sess, host):
    cmd = 'http://%s:8082/set_table' % host
    r = sess.post(cmd, auth=AUTH)
    return None if 200 != r.status_code else r.content
def overwrite_table(sess, host, cipher_id, private_pem, table):
    with open(table, 'rb') as f:
        body = f.read()
    with open(private_pem, 'rb') as f:
        p = f.read()
        privkey = rsa.PrivateKey.load_pkcs1(p)
        id = rsa.decrypt(cipher_id, privkey)
    cmd = 'http://%s:8082/set_table?id=%s' % (host, id)
    r = sess.post(cmd, body, auth=AUTH)
    if 200 != r.status_code:
        print r.headers['errorcode']
    return (id, 200 == r.status_code)

def set_table(sess, host, private_pem, table):
    cipher_id = get_identifier(sess, host)
    if not cipher_id:
        print 'get_identifier error'
        return
    return overwrite_table(sess, host, cipher_id, private_pem, table)

def loop(data):
    sess = requests.Session()
    for i in xrange(10):
        set_table(sess, data['host'], data['private_pem'], data['table'])

def test(argv):
    size = len(argv)
    if size < 2 or size > 5:
        return False
    if '-t' == argv[1]:
        sess = requests.Session()
        (id, rc) = set_table(sess, argv[2], argv[3], argv[4])
        print id
        print 'passed' if rc else 'failed'
        return True
    elif '-m' == argv[1]:
        tasks = 20
        pool = multiprocessing.Pool(tasks)
        pool.map(loop, [{'host': argv[2], 'private_pem': argv[3], 'table': argv[4]} for i in xrange(tasks)])
        pool.close()
        pool.join()
        return True
    return False

if __name__ == "__main__":
    if not test(sys.argv):
        manual()