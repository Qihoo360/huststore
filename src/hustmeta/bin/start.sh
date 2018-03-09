#!/bin/bash

server=$1

function start()
{
    if [ $# -lt 1 ]; then
        echo "`basename $0` [program]"
        return 0
    fi
    srv="$PWD/$server"
    nohup $srv > /dev/null 2>&1 &
}

start $*
