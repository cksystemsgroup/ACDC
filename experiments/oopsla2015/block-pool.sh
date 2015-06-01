#!/bin/bash

# EXPERIMENT SETTINGS

#name the allocators accordingly to their .so file in $ALLOCATOR_DIR

SCALLOCS="scalloc-hlab scalloc-tlab scalloc-eager scalloc-hugepage"
OTHERS="llalloc jemalloc hoard tcmalloc ptmalloc2 streamflow tbbmalloc_proxy"

#ALLOCATORS="$OTHERS"
#ALLOCATORS="scalloc-tlab"
#ALLOCATORS="$OTHERS scalloc-eager scalloc-clab scalloc-tlab"
ALLOCATORS="scalloc"

# TODO: compare w/ mem access

#OPTIONS="-a -s 4 -S 20 -d 30 -l 1 -L 5 -t 10000000 -O -T 5 -R 5 -N 5000 -C 500 -H 5000"
OPTIONS="-a -s 4 -S 9 -d 30 -l 1 -L 5 -t 1000000 -O -T 5 -R 5 -N 5000 -C 500 -H 1000 -A"
FACTOR1="-n"
FACTOR1_VALUES="1 2 4 6 8 10 20 30 39 40"
#FACTOR1_VALUES="1 4 16 40 80 256 1024 4096 8192 16384"
#FACTOR1_VALUES="1 4 16 40 80 256 1024"
#FACTOR1_VALUES="1 2 4 8 16 32 64 128 256 512 1024 2048 4096"
#FACTOR1_VALUES="1 2 4 8 16 32 64 128"
#FACTOR1_VALUES="1 2 4 8 16 32 64 128"
#FACTOR1_VALUES="16384"
FACTOR2=""
FACTOR2_VALUES=""
REPS=10
#if RELATIVE is set to 1, the the response will be divided by the value for x
RELATIVE=1

#TIMEOUT=1800

# OUTPUT SETTINGS
OUTPUT_DIR=`pwd`/data/oopsla2015/block-pool
# set to 1 for logscale gnuplot axes
#YLOGSCALE_ALLOC=1
#YLOGSCALE_ACCESS=1
#YLOGSCALE_FREE=1
#YLOGSCALE_MEMCONS=0
#YLOGSCALE_COMBINED=1

