#!/usr/bin/python
# email: yao050421103@gmail.com
import sys
import socket
import os
import json
import time
import datetime
import string

merge = lambda l: string.join(l, '\n')

def manual(): 
    print """
    usage:
        python make_conf.py [host_file] [module] [HA port] [backend port]
        
        [module]
            hustdbha
            hustmqha

    sample:
        python make_conf.py host.txt hustdbha 8082 8085
        python make_conf.py host.txt hustmqha 8080 8086
        """

def get_items(uri):
    with open(uri) as f:
        return filter(lambda s: len(s) > 0 and not s.startswith('#'), map(
            lambda s: s.split('\n')[0].split('\r')[0], f.readlines()))

def gen_hosts(host_file, port):
    return merge(['%s:%s' % (socket.gethostbyname(host), port) for host in get_items(host_file)])

def get_json_data(uri):
    with open(uri) as f:
        return json.load(f)
    
def update_conf(ha_port, host_file, prefix):
    cf = os.path.join(prefix, 'nginx.json')
    nginx_conf = get_json_data(cf)
    nginx_conf['listen'] = ha_port
    nginx_conf['proxy']['backends'] = get_items(host_file)
    with open(cf, 'w') as f:
        json.dump(nginx_conf, f, indent=4)
    script = os.path.join(prefix, 'genconf.py')
    os.system('python %s %s' % (script, cf))

def gen_backend_host_file(host_file, backend_port, prefix):
    file_name = 'hosts_%s.txt' % datetime.datetime.now().strftime('%Y%m%d%H%M%S')
    uri = os.path.join(prefix, file_name)
    with open(uri, 'w') as f:
        f.write(gen_hosts(host_file, backend_port))
    return uri

def gen_ngx_conf(host_file, ha_port, backend_port, prefix):
    backend_host_file = gen_backend_host_file(host_file, backend_port, prefix)
    update_conf(ha_port, backend_host_file, prefix)
    return backend_host_file

def gen_hustdbha_conf(host_file, ha_port, backend_port, prefix):
    backend_host_file = gen_ngx_conf(host_file, ha_port, backend_port, prefix)
    script = os.path.join(prefix, 'gen_table.py')
    table_file = os.path.join(prefix, 'hustdbtable.json')
    os.system('python %s %s %s' % (script, backend_host_file, table_file))
    os.remove(backend_host_file)
    return True

def gen_hustmqha_conf(host_file, ha_port, backend_port, prefix):
    backend_host_file = gen_ngx_conf(host_file, ha_port, backend_port, prefix)
    os.remove(backend_host_file)
    return True

def gen_conf(host_file, module, ha_port, backend_port):
    if 'hustdbha' == module:
        return gen_hustdbha_conf(host_file, int(ha_port), backend_port, 'hustdb/ha/nginx/conf')
    elif 'hustmqha' == module:
        return gen_hustmqha_conf(host_file, int(ha_port), backend_port, 'hustmq/ha/nginx/conf')
    return False

def parse_shell(argv):
    return gen_conf(argv[1], argv[2], argv[3], argv[4]) if len(argv) == 5 else False

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()