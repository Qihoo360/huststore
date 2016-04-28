#/usr/bin/python
import sys
import os
import platform
import os.path
import json
import string

FILTER = '@null_string_place_holder'
merge = lambda l: string.join(filter(lambda item: FILTER != item, l), '\n')
spaces = lambda n: string.join([' ' for i in xrange(0, n)], '')
tabs = [spaces(0), spaces(4), spaces(8), spaces(12), spaces(16), spaces(20)]

KEY = 0
VAL = 1

LISTEN = 'listen%s' % tabs[5]
FIX_SIZE = len(LISTEN)

def manual(): 
    print """
    usage sample:
        python genconf.py nginx.json
        """

def gen(conf, path):
    __global = [
        'worker_processes  1;',
        'daemon on;',
        'master_process on;',
        ''
        ]
    __events = lambda conf: [
        'events {',
        '    use epoll;',
        '    multi_accept on;',
        '    worker_connections  %d;' % conf['worker_connections'],
        '}',
        ''
        ]
        
    __has_proxy_auth = lambda o: 'proxy' in o and 'auth' in o['proxy']
    __proxy = lambda conf, cmd: merge([
        '%slocation %s {' % (tabs[2], cmd),
        '%sproxy_pass http://backend;' % tabs[3],
        '%sproxy_http_version 1.1;' % tabs[3],
        '%sproxy_set_header Connection "Keep-Alive";' % tabs[3],
        '%sproxy_set_header Authorization "Basic %s";' % (
            tabs[3], conf['proxy']['auth']) if __has_proxy_auth(conf) else FILTER,
        '%sproxy_connect_timeout %s;' % (tabs[3], conf['proxy']['proxy_connect_timeout']),
        '%sproxy_send_timeout %s;' % (tabs[3], conf['proxy']['proxy_send_timeout']),
        '%sproxy_read_timeout %s;' % (tabs[3], conf['proxy']['proxy_read_timeout']),
        '%sproxy_buffer_size %s;' % (tabs[3], conf['proxy']['proxy_buffer_size']),
        '%sproxy_buffers 2 %s;' % (tabs[3], conf['proxy']['proxy_buffer_size']),
        '%sproxy_busy_buffers_size %s;' % (tabs[3], conf['proxy']['proxy_buffer_size']),
        '%s}' % tabs[2]
        ])
    __backends = lambda backends: merge(map(
        lambda s: '%sserver %s;' % (tabs[2], s), backends
        )) if len(backends) > 0 else '%s#server host:port;' % tabs[2]
    __prefix = lambda conf, cmd: '#' if 'auth_filter' in conf and cmd in conf['auth_filter'] else ''
    __router = lambda conf: lambda cmd: merge([
        '%slocation /%s {' % (tabs[2], cmd),
        '%s%s;' % (tabs[3], conf['module']),
        '%s%shttp_basic_auth_file %s;' % (
            tabs[3], __prefix(conf, cmd), conf['http_basic_auth_file']
            ) if 'http_basic_auth_file' in conf else FILTER,
        '%s}' % tabs[2]
        ])
    __main_conf = lambda mcf: merge(map(
        lambda cf: '%s%s%s%s;' % (tabs[2], cf[KEY], spaces(FIX_SIZE - len(cf[KEY])), cf[VAL]), mcf))
    __proxy_has_key = lambda key: lambda o: 'proxy' in o and key in o['proxy']
    __has_backends = __proxy_has_key('backends')
    __has_proxy_cmds = __proxy_has_key('proxy_cmds')
    __gen_health_check = lambda items: merge(map(lambda item: (
        '        %s;' % item), items))
    __gen_upstream = lambda conf: merge([
        '    upstream backend {',
        '        customized_selector;',
        __backends(conf['proxy']['backends']),
        __gen_health_check(conf['proxy']['health_check']),
        '        keepalive %d;' % conf['keepalive'] if 'keepalive' in conf else FILTER,
        '    }',
        '',
        ]) if __has_backends(conf) else FILTER
    __http = lambda conf: [
        'http {',
        '    include                      mime.types;',
        '    default_type                 application/octet-stream;',
        '',
        '    sendfile                     on;',
        '    keepalive_timeout            %d;' % conf['keepalive_timeout'] if 'keepalive_timeout' in conf else FILTER,
        '',
        '    client_body_timeout          10;',
        '    client_header_timeout        10;',
        '',
        '    client_header_buffer_size    1k;',
        '    large_client_header_buffers  4  4k;',
        '    output_buffers               1  32k;',
        '    client_max_body_size         64m;',
        '    client_body_buffer_size      2m;',
        '',
        __gen_upstream(conf),
        '    server {',
        '        %s%d;' % (LISTEN, conf['listen']),
        '        #server_name              hostname;',
        '        access_log                /dev/null;',
        '        error_log                 /dev/null;',
        '        chunked_transfer_encoding off;',
        '        keepalive_requests        %d;' % conf['keepalive'] if 'keepalive' in conf else FILTER,
        __main_conf(conf['main_conf']) if 'main_conf' in conf else FILTER,
        '',
        '        location /status.html {',
        '            root %s;' % ('/var/www/html' if not 'nginx_root' in conf else conf['nginx_root']),
        '        }',
        '',
        merge(map(__router(conf), conf['local_cmds'])),
        '',
        merge(map(lambda uri: __proxy(conf, uri), conf['proxy']['proxy_cmds'])
            ) if __has_proxy_cmds(conf) else FILTER,
        '    }',
        '}'
        ]
    with open(path, 'w') as f:
        f.writelines(merge([
            merge(__global),
            merge(__events(conf)),
            merge(__http(conf))
            ]))

def parse_shell(argv):
    size = len(argv)
    if size > 2:
        return False
    path = argv[1] if 2 == size else 'nginx.json'
    output = '%s%snginx.conf' % (
        os.path.dirname(os.path.abspath(path)),
        '\\' if ("Windows" == platform.system()) else '/'
        ) if 2 == size else 'nginx.conf'
    with open(path, 'r') as f:
        gen(json.load(f), output)
    return True

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()
