#!/bin/bash

# EXPERIMENT SETTINGS

#name the allocators accordingly to their .so file in $ALLOCATOR_DIR
ALLOCATORS="jemalloc llalloc scalloc-core-local scalloc-eager-reuse scalloc-eager scalloc-lazy-init scalloc-static"
#ALLOCATORS="scalloc-core-local scalloc-eager-reuse scalloc-eager scalloc-lazy-init scalloc-static"
#OPTIONS="-a -s 4 -S 20 -d 30 -l 1 -L 5 -t 10000000 -O -T 5 -R 5 -N 5000 -C 500 -H 5000"
OPTIONS="-a -s 4 -S 14 -d 30 -l 1 -L 5 -t 1000000 -O -T 5 -R 5 -N 12000 -C 500 -H 7000"
FACTOR1="-n"
FACTOR1_VALUES="1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192"
FACTOR2=""
FACTOR2_VALUES=""
REPS=2
#if RELATIVE is set to 1, the the response will be divided by the value for x
RELATIVE=1

# OUTPUT SETTINGS
OUTPUT_DIR=`pwd`/data/block-pool
# set to 1 for logscale gnuplot axes
YLOGSCALE_ALLOC=1
YLOGSCALE_ACCESS=1
YLOGSCALE_FREE=1
YLOGSCALE_MEMCONS=0
YLOGSCALE_COMBINED=1

