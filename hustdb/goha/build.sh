#!/bin/bash

option=$1

function build_goha()
{
	export GOPATH=$(cd "$(dirname "$0")"; pwd)
    if [ "$option"x = "-c"x ]; then
        declare -a dirs=(\
            "bin/goha" \
            "bin/conf")
        for dir in "${dirs[@]}"
        do
            echo "rm -rf $dir"
            rm -rf $dir
        done
    fi
    declare -a projects=(\
        "goha")
    for project in "${projects[@]}"
    do
        echo "build $project ..."
        go build -gcflags "-N -l" -o "bin/$project" 
    done
    cp -r conf/ bin
}

build_goha $*
