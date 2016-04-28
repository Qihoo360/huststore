平滑升级
--

### 平滑升级脚本样例 ###

路径：`hustmq/ha/upgrade.sh`

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

使用方法：

    sh upgrade.sh jobs

[上一级](index.md)

[根目录](../../index.md)