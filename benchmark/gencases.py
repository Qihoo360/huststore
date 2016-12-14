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

def gen_init(max_requests, loop_file, tail):
    return tpls['init'].substitute({'var_max_requests': max_requests, 'var_loop': loop_file, 'var_tail': tail})
def gen_request(make_request):
    return tpls['request'].substitute({'var_make_request': make_request})
def gen_done(loop_file, head, mid, tail):
    return substitute_tpls(tpls['done'], {'var_loop': loop_file, 'var_head': head, 'var_mid': mid, 'var_tail': tail})
def gen_benchmark(method, load_tpl, init, request, done):
    return merge([
        tpls['utils'].substitute({'var_method': method, 'var_auth': AUTH}),
        load_tpl,
        tpls['setup'].template,
        init,
        request,
        tpls['response'].template,
        done
        ])

def gen_set_done(loop_file, requests_file):
    return gen_done(loop_file,
        tpls['done_head'].substitute({'var_requests_file': requests_file}),
        tpls['done_mid'].template, 
        tpls['done_tail'].template
        )

def gen_http_set(uri, tpl_key, loop_file, requests_file):
    return gen_benchmark(
        'POST',
        tpls['load_body'].template,
        gen_init(MAX_REQUESTS, loop_file, tpls['init_set'].template),
        gen_request(tpls[tpl_key].substitute({'var_uri': uri})),
        gen_set_done(loop_file, requests_file)
        )
def gen_http_get(uri, tpl_key, loop_file, requests_file):
    return gen_benchmark(
        'GET',
        tpls['load_requests'].template,
        gen_init('get_requests("%s", id)' % requests_file, loop_file, tpls['init_get'].template),
        gen_request(tpls[tpl_key].substitute({'var_uri': uri})),
        gen_done(loop_file, FILTER, FILTER, FILTER)
        )
def gen_http_post(uri, tpl_key, loop_file, requests_file):
    return gen_benchmark(
        'POST',
        merge([tpls['load_body'].template, tpls['load_requests'].template]),
        gen_init('get_requests("%s", id)' % requests_file, loop_file, tpls['init_post'].template),
        gen_request(tpls[tpl_key].substitute({'var_uri': uri})),
        gen_set_done(loop_file, requests_file)
        )

def gen(wrk, out):
    items = json.loads(tpls['uri_table'].template)
    SRV = 0
    URI = 1
    TPL = 2
    FUNC = 3
    OUT = 4
    func_dict = { 'http_set': gen_http_set, 'http_get': gen_http_get, 'http_post': gen_http_post }
    lines = []
    for item in items:
        if None == item:
            continue
        if 'outputs' in wrk:
            if not item[OUT] in wrk['outputs']:
                continue
        write_file(os.path.join(out, item[OUT]), func_dict[item[FUNC]](item[URI], get_tpl_key(item[TPL]), 'loop.txt', 'thread.requests'))
        case = os.path.splitext(item[OUT])[0]
        script = '%s.sh' % case
        log = '%s.log' % case
        output = '%s.json' % case
        write_file(os.path.join(out, script), 'wrk -t%d -c%d -d%s -T%s --latency -s "%s" "http://%s"' % (
            wrk['wrk']['threads'], 
            wrk['wrk']['connections'], 
            wrk['wrk']['duration'], 
            wrk['wrk']['timeout'], 
            item[OUT], 
            wrk['srv'][item[SRV]]))
        for item in ['runcase.py', 'analyze.py']:
            shutil.copy(item, out)
        lines.append('python runcase.py %s %d %s %s' % (script, wrk['wrk']['repeated'], wrk['filter'], log))
        lines.append('python analyze.py %s %s %s' % (log, wrk['filter'], output))
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