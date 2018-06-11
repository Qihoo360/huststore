#!/usr/bin/python
#author: jobs
#email: yao050421103@gmail.com
import sys
import os.path
import datetime
import string
import json
import base64
import shutil

FILTER = '@null_string_place_holder'
join_lines = lambda lines, dim: string.join(filter(lambda item: -1 == item.find(FILTER), lines), dim)
merge = lambda lines: join_lines(lines, '\n')
AUTH = base64.b64encode('huststore:huststore')
MAX_REQUESTS = '1048576'
join_name = lambda prefix: lambda name: ''.join([prefix, name])

def manual(): 
    print """
    usage:
        python gencases.py [conf] [output]
    sample:
        python gencases.py wrk.json .
        """

def write_file(url, data):
    with open(url, 'w') as f:
        f.writelines(data)
def merge_array(arr):
    out = []
    for item in arr:
        out.extend(item)
    return out

substitute_tpls = lambda tpls, vars: join_lines([tpl.substitute(vars) for tpl in tpls], '')
get_tpl_key = lambda s: os.path.splitext(s)[0].replace('/', '_')
def load_templates():
    cwd = os.path.split(os.path.realpath(__file__))[0]
    tpl_path = os.path.join(cwd, 'tpl')
    with open(os.path.join(tpl_path, 'tpls.json')) as f:
        cf = json.load(f)
        templates = {}
        for item in cf['tpls']:
            with open(os.path.join(tpl_path, item)) as f:
                key = get_tpl_key(item)
                val = string.Template(f.read())
                templates[key] = val
        for item in cf['tpl_lines']:
            with open(os.path.join(tpl_path, item)) as f:
                key = get_tpl_key(item)
                templates[key] = [string.Template(line) for line in f]
        return templates

tpls = load_templates()

def gen_init(max_requests, tail):
    return tpls['init'].substitute({'var_max_requests': max_requests, 'var_tail': tail})
def gen_request(make_request):
    return tpls['request'].substitute({'var_make_request': make_request})
def gen_done(loop_file, distribution, head, mid, tail):
    return substitute_tpls(tpls['done'], {
        'var_loop': loop_file, 
        'var_distribution': string.join(distribution, ', '), 
        'var_head': head, 
        'var_mid': mid, 
        'var_tail': tail
        })
def gen_benchmark(loop_file, status_file, method, load_tpl, init, request, done):
    return merge([
        tpls['utils'].substitute({
            'var_loop': loop_file, 
            'var_status_file': status_file, 
            'var_method': method, 
            'var_auth': AUTH}),
        load_tpl,
        tpls['setup'].template,
        init,
        request,
        tpls['response'].template,
        done
        ])
def gen_set_done(loop_file, distribution):
    return gen_done(
        loop_file,
        distribution,
        tpls['done_head'].template,
        tpls['done_mid'].template, 
        tpls['done_tail'].template
        )

def gen_http_set(loop_file, status_file, distribution, uri, tpl_key):
    return gen_benchmark(
        loop_file,
        status_file,
        'POST',
        tpls['load_body'].template,
        gen_init(MAX_REQUESTS, tpls['init_set'].template),
        gen_request(tpls[tpl_key].substitute({'var_uri': uri})),
        gen_set_done(loop_file, distribution)
        )
def gen_http_get(loop_file, status_file, distribution, uri, tpl_key):
    return gen_benchmark(
        loop_file,
        status_file,
        'GET',
        tpls['load_requests'].template,
        gen_init('get_requests(status_file, id)', tpls['init_get'].template),
        gen_request(tpls[tpl_key].substitute({'var_uri': uri})),
        gen_done(loop_file, distribution, FILTER, FILTER, FILTER)
        )
def gen_http_post(loop_file, status_file, distribution, uri, tpl_key):
    return gen_benchmark(
        loop_file,
        status_file,
        'POST',
        merge([tpls['load_body'].template, tpls['load_requests'].template]),
        gen_init('get_requests(status_file, id)', tpls['init_post'].template),
        gen_request(tpls[tpl_key].substitute({'var_uri': uri})),
        gen_set_done(loop_file, distribution)
        )

def get_duration(uri, wrk):
    if not 'special_duration' in wrk:
        return wrk['wrk']['duration']
    if uri in wrk['special_duration']:
        return wrk['special_duration'][uri]
    return wrk['wrk']['duration']

def gen(wrk, out):
    items = json.loads(tpls['uri_table'].template)
    SRV = 0
    URI = 1
    TPL = 2
    FUNC = 3
    OUT = 4
    func_dict = { 'http_set': gen_http_set, 'http_get': gen_http_get, 'http_post': gen_http_post }
    lines = []
    loop_file = wrk['status']['loop_file']
    for item in items:
        if None == item:
            continue
        if 'outputs' in wrk:
            if not item[OUT] in wrk['outputs']:
                continue
        write_file(os.path.join(out, item[OUT]), func_dict[item[FUNC]](
            loop_file,
            wrk['status']['status_file'],
            wrk['latency_distribution'],
            item[URI], 
            get_tpl_key(item[TPL])
            ))
        case = os.path.splitext(item[OUT])[0]
        script = '%s.sh' % case
        log = '%s.log' % case
        output = '%s.json' % case
        write_file(os.path.join(out, script), 'wrk -t%d -c%d -d%s -T%s --latency -s "%s" "http://%s"' % (
            wrk['wrk']['threads'], 
            wrk['wrk']['connections'], 
            get_duration(item[URI], wrk), 
            wrk['wrk']['timeout'], 
            item[OUT], 
            wrk['srv'][item[SRV]]))
        for item in ['runcase.py', 'analyze.py']:
            shutil.copy(item, out)
        lines.append('rm -f %s' % loop_file)
        lines.append('python runcase.py %s %d %s %s' % (script, wrk['wrk']['loop'], wrk['separator'], log))
        lines.append('python analyze.py %s %s %s' % (log, wrk['separator'], output))
    write_file(os.path.join(out, 'benchmark.sh'), merge(lines))
    return True

def parse_shell(argv):
    size = len(argv)
    if 3 != size:
        return False
    with open(argv[1]) as f:
        return gen(json.load(f), argv[2])
if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()