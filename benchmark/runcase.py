#!/usr/bin/python
#author: jobs
#email: yao050421103@gmail.com
import sys
import os.path

def manual(): 
    print """
    usage:
        python runcase.py [script] [loop] [separator] [output]
    sample:
        python runcase.py hustdb_put.sh 5 @huststore_benchmark hustdb_put.log
        """

def execute(cmd):
    with os.popen(cmd) as f:
        return f.read()

def load_file(uri):
    with open(uri) as f:
        return f.read()

def runcase(script, loop, ft, output):
    cmd = load_file(script)
    with open(output, 'w') as f:
        for i in xrange(loop):
            f.write(ft)
            f.write('\n')
            f.write(execute(cmd))
            f.write('\n')
    return True

def parse_shell(argv):
    return runcase(argv[1], int(argv[2]), argv[3], argv[4]) if 5 == len(argv) else False

if __name__ == "__main__":
    if not parse_shell(sys.argv):
        manual()