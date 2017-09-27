#!/bin/sh
#graphname partition destDir
if [ 3 -eq $# ];then
    echo the format of parameters is: input_graph_file partition destDir memSize\(MB\).
    exit 1
fi
if [ ! -f $1 ];then
    echo $1 does not exist
    exit 1
fi
if [ ! -d $3 ];then
    echo $3 does not exist
    exit 1
fi

graph=$1
part=$2
dir=$3
memory=$4
filename=${graph##*/}

./generate.sh $1 $2 $3 $4
./generate.sh $1-00 $2 $3 $4
./generate.sh $1-11 $2 $3 $4

#remap grid-01 && grid-10
./bin/convert -t remap -g $1 -d $3 -m $4

#get degree info for grid-01 and grid-10(do not need mmap)
./bin/convert -t statistics -g $1 -d $3 -m 1024

#get degree info for grid-00-01 and grid-00-10(do not need mmap)
./bin/convert -t statistics -g $1-00 -d $3 -m 1024

#get degree info for grid-11-01 and grid-11-10(do not need mmap)
./bin/convert -t statistics -g $1-11 -d $3 -m 1024
