#/usr/bin/python
# author: jobs
# email: yao050421103@163.com
import sys
import os
import re
import time
import subprocess

def get_pids(pname):
    ptn = re.compile("\s+")
    p1 = subprocess.Popen(["ps", "aux"], stdout=subprocess.PIPE)
    p2 = subprocess.Popen(["grep", pname], stdin=p1.stdout, stdout=subprocess.PIPE)
    p1.stdout.close()
    output = p2.communicate()[0]
    lines = output.strip().split("\n")
    pids = []
    for line in lines:
        items = ptn.split(line)
        if 'grep' == items[-2]:
            continue
        pids.append(items[1])
    return pids

def stop():
    pids = get_pids("hustdbsync")
    if len(pids) > 0:
        os.system('./hustdbsync -q')
        while 1:
            time.sleep(0.1)
            pids = get_pids("hustdbsync")
            if len(pids) < 1:
                break

if __name__ == "__main__":
    stop()