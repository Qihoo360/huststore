#!/bin/bash
export PATH=/bin:/usr/bin:/usr/sbin:/sbin:${PATH}

help(){
echo "Usage:"
echo "       $0    # prefix default to /opt/huststore "
echo "       $0 --prefix </opt/huststore>    # specify prefix"
echo "       $0 --help|-h"
return 1
}

if ! options=$(getopt -o hd  -l help,prefix: -- "$@")
then
    help
    exit 1
fi

eval set -- "$options"

while [ $# -gt 0 ]
do
    case $1 in
    -h|--help) help; exit 1 ;;
    -p|--prefix) prefix="$2" ; shift;;
    (--) shift; break;;
    (-*) echo "$0: error - unrecognized option $1" 1>&2; help; exit 1;;
    (*) break;;
    esac
    shift
done

export HUSTSTORE_TOP=${prefix:-/opt/huststore}

export PREFIX_3RD=${HUSTSTORE_TOP}/3rd
export PREFIX_HUSTDB=${HUSTSTORE_TOP}/hustdb
export PREFIX_HUSTDBSYNC=${HUSTSTORE_TOP}/hustdbsync
export PREFIX_HUSTDBHA=${HUSTSTORE_TOP}/hustdbha
export PREFIX_HUSTMQ=${HUSTSTORE_TOP}/hustmq
export PREFIX_HUSTMQHA=${HUSTSTORE_TOP}/hustmqha


for oldf in \
  third_party/build.sh.in \
  hustdb/ha/nginx/Config.sh.in \
  hustdb/ha/nginx/conf/nginx.json.in \
  hustdb/ha/nginx/conf/zlog.conf.in \
  hustmq/ha/nginx/conf/nginx.json.in \
  hustmq/ha/nginx/Config.sh.in \
  hustdb/db/Config.sh.in \
  hustdb/sync/Config.sh.in \
  hustdb/sync/module/sync_server.json.in \
  hustdb/sync/module/zlog.conf.in \
; do
  newf=${oldf%.in}
  sed \
    "s,@@PREFIX_3RD@@,${PREFIX_3RD},g;
    s,@@PREFIX_HUSTDB@@,${PREFIX_HUSTDB},g;
    s,@@PREFIX_HUSTDBSYNC@@,${PREFIX_HUSTDBSYNC},g;
    s,@@PREFIX_HUSTDBHA@@,${PREFIX_HUSTDBHA},g;
    s,@@PREFIX_HUSTMQ@@,${PREFIX_HUSTMQ},g;
    s,@@PREFIX_HUSTMQHA@@,${PREFIX_HUSTMQHA},g;
    " ${oldf} > ${newf}
    [[ $(basename ${newf}) == Config.sh ]] && chmod +x ${newf}
done

