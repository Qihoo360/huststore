#!/bin/bash

option=$1
cwd=$(dirname $(readlink -f $0))
export GOPATH=$(dirname $cwd)

function clean()
{
    declare -a dirs=(\
        "installation" \
        "bin/data" \
        "bin/logs" \
        "bin/hustmeta" \
        "bin/conf")
    for dir in "${dirs[@]}"
    do
        echo "rm -rf $dir"
        rm -rf $dir
    done
}

function mkdirs()
{
    declare -a dirs=(\
        "installation" \
        "bin/data" \
        "bin/logs")

    for dir in "${dirs[@]}"
    do
        echo "mkdir $dir"
        test -d $dir || (mkdir -p $dir && chmod 777 $dir)
    done
}

function post_build()
{
    mkdirs
    echo "make installation"
    cp -r conf bin
    cp -r bin installation
    cd installation
    mv bin hustmeta
    tar -zcf hustmeta.tar.gz hustmeta
    rm -rf hustmeta
}

function build()
{
    echo "build hustmeta ..."
    go build -gcflags "-N -l" -o "bin/hustmeta" "hustmeta"
    post_build
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
