#!/bin/bash

option=$1
cwd=$(dirname $(readlink -f $0))
root="/data"
install_dir="$root/huststore"
srv_dir="$install_dir/hustmeta"

function build()
{
    echo "building..."
    chmod a+x build.sh
    ./build.sh
}

function mkdirs()
{
    declare -a dirs=(\
        $root \
        $install_dir)

    for dir in "${dirs[@]}"
    do
        echo "mkdir $dir"
        test -d $dir || (mkdir -p $dir && chmod 777 $dir)
    done
}

function rename_confs()
{
    srv_dir=$1
    declare -a oldconfs=(\
        "$srv_dir/conf/hustmeta.json" \
        "$srv_dir/conf/seelog.xml")

    for oldconf in "${oldconfs[@]}"
    do
        echo "check $oldconf"
        test ! -f $oldconf || cp $oldconf "$oldconf.old"
    done
}

function install()
{
    install_dir=$1
    cp "installation/hustmeta.tar.gz" $install_dir
    cd $install_dir
    tar -zxf hustmeta.tar.gz -C .
    rm -f hustmeta.tar.gz
}

function main()
{
    if [ "$option"x = "-b"x ]; then
        build
    fi
    
    cd $cwd

    mkdirs
    rename_confs $srv_dir
    install $install_dir

    echo "done"
}

main $*
