#!/usr/bin/python
#author: jobs
#email: yao050421103@gmail.com
import sys
import os.path
import re
import json

KEY = 0
PTN = 1
VAL = 2
AVG = 3
REP = 4

get_float_avg = lambda items: sum(items) / float(len(items))
get_stats_pattern = lambda key: ''.join([])

def manual(): 
    print """
    usage:
        python analyze.py [log] [filter] [output]
    sample:
        python analyze.py hustdb_put.log @huststore_benchmark hustdb_put.json
        """

def load_file(uri):
    with open(uri) as f:
        return f.read()

def get_avg_success_rate(items):
    reqs = sum([item[0] for item in items])
    errs = sum([item[1] for item in items])
    rate = float(reqs - errs) * float(100) / float(reqs)
    return '%.2f%% [%d/%d (errs/reqs)]' % (rate, errs, reqs)
def get_avg_latency(items):
    avgdict = {}
    for item in items:
        key = item[0]
        latency = item[1]
        if not key in avgdict:
            avgdict[key] = [latency]
        else:
            avgdict[key].append(latency)
    latency = {}
    for key in avgdict:
        latency[key] = get_float_avg(avgdict[key])
    return latency
def get_avg_thread_stats(unit):
    def get_avg(items):
        avgs = [[],[],[],[]]
        size = len(avgs)
        for item in items:
            for i in xrange(size):
                avgs[i].append(item[i])
        return {
            'Avg(%s)' % unit: get_float_avg(avgs[0]),
            'Stdev(%s)' % unit: get_float_avg(avgs[1]),
            'Max(%s)' % unit: get_float_avg(avgs[2]),
            '+/- Stdev(%)': get_float_avg(avgs[3])
            }
    return get_avg
def init_patterns():
    return [
        [
            'Requests/sec', 
            re.compile('^Requests/sec:\s+(?P<qps>[\d|\.]+)$'), 
            lambda d: float(d['qps']),
            get_float_avg,
            False
        ],
        [
            'Transfer/sec(MB)', 
            re.compile('^Transfer/sec:\s+(?P<io>[\d|\.]+)MB$'), 
            lambda d: float(d['io']),
            get_float_avg,
            False
        ],
        [
            'Success rate', 
            re.compile('^\[summary\]\s+loop:\s+[\d]+,\s+requests:\s+(?P<reqs>[\d]+),\s+fails:\s+(?P<errs>[\d]+)$'), 
            lambda d: [int(d['reqs']), int(d['errs'])],
            get_avg_success_rate,
            False
        ],
        [
            'Latency Distribution(ms)', 
            re.compile('^\s+(?P<key>[\d]+)%\s+(?P<latency>[\d|\.]+)ms$'), 
            lambda d: [''.join([d['key'], '%']), float(d['latency'])],
            get_avg_latency,
            True
        ],
        [
            'Thread Latency', 
            re.compile('^\s+Latency\s+(?P<avg>[\d|\.]+)ms\s+(?P<stdev>[\d|\.]+)ms\s+(?P<max>[\d|\.]+)ms\s+(?P<rate>[\d|\.]+)%$'),
            lambda d: [float(d['avg']), float(d['stdev']), float(d['max']), float(d['rate'])],
            get_avg_thread_stats('ms'),
            False
        ],
        [
            'Thread QPS', 
            re.compile('^\s+Req/Sec\s+(?P<avg>[\d|\.]+)k\s+(?P<stdev>[\d|\.]+)k\s+(?P<max>[\d|\.]+)k\s+(?P<rate>[\d|\.]+)%$'),
            lambda d: [float(d['avg']), float(d['stdev']), float(d['max']), float(d['rate'])],
            get_avg_thread_stats('k'),
            False
        ]
        ]
def match_pattern(ptns, line, results):
    for ptn in ptns:
        m = ptn[PTN].match(line)
        if None == m:
            continue
        key = ptn[KEY]
        val = ptn[VAL](m.groupdict())
        if not key in results:
            results[key] = [val]
        else:
            results[key].append(val)
        if not ptn[REP]:
            ptns.remove(ptn)
        break
def get_avg_dict(ptns):
    avgs = {}
    for item in ptns:
        avgs[item[KEY]] = item[AVG]
    return avgs
def analyze(uri, ft, output):
    patterns = init_patterns()
    avgdict = get_avg_dict(patterns)
    cwd = os.path.split(os.path.realpath(__file__))[0]
    filename = os.path.splitext(os.path.basename(uri))[0]
    data = load_file(uri)
    results = {}
    for item in data.split(ft):
        ptns = [p for p in patterns]
        for line in item.split('\n'):
            match_pattern(ptns, line, results)
    stats = {}
    for key in results:
        stats[key] = avgdict[key](results[key])
    stats['Thread Stats'] = {'Latency': stats.pop('Thread Latency'),'Req/Sec': stats.pop('Thread QPS')}    
    with open(output, 'w') as f:
        json.dump(stats, f, indent = 4)
    return True

def parse_shell(argv):
    return analyze(argv[1], argv[2], argv[3]) if 4 == len(argv) else False

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()