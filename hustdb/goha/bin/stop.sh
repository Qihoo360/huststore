#!/bin/bash

server=$1

function stop()
{
    if [ $# -lt 1 ]; then
        echo "`basename $0` [program]"
        return 0
    fi
    srv="$PWD/$server"
    ps gaux | grep $srv | grep -v grep | awk '{print $2}' | xargs kill -9
}

stop $*
