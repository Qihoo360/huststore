Smooth Upgrade
--

### Script example for smooth upgrade###

Path: `hustdb/ha/upgrade.sh`

	#!/bin/bash
	
	user=$1
	
	function upgrade()
	{
	    if [ $# -lt 1 ]; then
	        echo "`basename $0` [user]"
	        return 0
	    fi
	    oldpid=`ps aux | grep nginx | grep $user | awk 'BEGIN {FS=" "} $11 == "nginx:" && $12 == "master" {print $2}'`
	    if [ "$oldpid"x = ""x ]; then
	        echo "no nginx running"
	        return 0
	    fi
	    echo "kill -USR2 $oldpid..."
	    kill -USR2 $oldpid
	    sleep 1s
	    echo "kill -WINCH $oldpid..."
	    kill -WINCH $oldpid
	    sleep 1s
	    echo "kill -QUIT $oldpid..."
	    kill -QUIT $oldpid
	    sleep 1s
	    echo "upgrade successful!"
	    return 1
	}
	
	upgrade $*

Usage:

    sh upgrade.sh jobs # jobs 代表服务运行的账户名称

[Previous page](../ha.md)

[Home](../../index.md)