#!/bin/bash

option=$1
export GOPATH=$(dirname $(readlink -f $0))

function clean()
{
    declare -a dirs=(\
        "bin/hustmeta")
    for dir in "${dirs[@]}"
    do
        echo "rm -rf $dir"
        rm -rf $dir
    done
}

function build()
{
    echo "build hustmeta ..."
    go build -gcflags "-N -l" -o "bin/hustmeta"
}

function main()
{
    if [ "$option"x = "-c"x ]; then
        clean
        return 0
    fi
    
    build
}

main $*
