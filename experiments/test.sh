#!/bin/bash

# EXPERIMENT SETTINGS

#name the allocators accordingly to their .so file in $ALLOCATOR_DIR
#ALLOCATORS="scalloc-core-local scalloc llalloc jemalloc"
#ALLOCATORS="scalloc-core-local scalloc llalloc jemalloc"
ALLOCATORS="nulloc"
OPTIONS="-a -s 4 -S 15 -d 50 -l 1 -L 5 -t 512000 -O -T 10 -R 10 -H 6000000000 -C 1000000 -N 1000000"
#OPTIONS="-a -s 4 -S 10 -d 100 -l 1 -L 10 -t 500000 -O -T 50 -R 50"
FACTOR1="-n"
FACTOR1_VALUES="1024 2048 4096 8192 13000"
#FACTOR1_VALUES="512"
FACTOR2=""
FACTOR2_VALUES=""
REPS=2
#if RELATIVE is set to 1, the the response will be divided by the value for x
RELATIVE=1

# OUTPUT SETTINGS
OUTPUT_DIR=`pwd`/data/testdata
# set to 1 for logscale gnuplot axes
YLOGSCALE_ALLOC=1
YLOGSCALE_FREE=1
YLOGSCALE_MEMCONS=0

