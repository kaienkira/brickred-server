#!/bin/sh

usage()
{
    echo 'usage: config.sh [options]'
    echo '-h --help            print usage'
    echo '--prefix=<prefix>    install prefix'
    echo '--build-test         build test programs'
    exit 1
}

brickred_install_prefix='/usr/local'
brickred_compile_flag=
brickred_link_flag=
brickred_build_test='no'

options=`getopt -o h -l \
help,\
prefix:,\
build-test\
 -- "$@"`
eval set -- "$options"

while [ $# -gt 0 ]
do
    case "$1" in
    -h|--help) usage;;
    --prefix) brickred_install_prefix=$2; shift;;
    --build-test) brickred_build_test=yes;;
    --) shift; break;;
    *) usage;;
    esac
    shift
done

# check compiler
which g++ >/dev/null 2>&1
if [ $? -ne 0 ]
then
    echo 'can not find g++'
    exit 1
fi

# check make
which make >/dev/null 2>&1
if [ $? -ne 0 ]
then
    echo 'can not find make'
    exit 1
fi

# output
echo "BRICKRED_INSTALL_PREFIX = $brickred_install_prefix" >config.mak
echo "BRICKRED_COMPILE_FLAG = $brickred_compile_flag" >>config.mak
echo "BRICKRED_LINK_FLAG = $brickred_link_flag" >>config.mak
echo "BRICKRED_BUILD_TEST = $brickred_build_test" >>config.mak
