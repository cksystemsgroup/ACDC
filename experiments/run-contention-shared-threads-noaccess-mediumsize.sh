#!/bin/bash

# EXPERIMENT SETTINGS

#name the allocators accordingly to their .so file in $ALLOCATOR_DIR
ALLOCATORS="jemalloc llalloc ptmalloc2 tbbmalloc_proxy tcmalloc streamflow hoard scalloc scalloc-eager"
OPTIONS="-a -s 4 -S 20 -d 20 -l 1 -L 5 -t 10000000 -N 40000 -C 40000 -O -T 100 -R 100 -H 500000"
FACTOR1="-n"
FACTOR1_VALUES="1 2 4 8 10 20 40 60 80"
FACTOR2=""
FACTOR2_VALUES=""
REPS=5
#if RELATIVE is set to 1, the the response will be divided by the value for x
RELATIVE=1

# OUTPUT SETTINGS
OUTPUT_DIR=`pwd`/data/contention-shared-threads-noaccess-mediumsize
# set to 1 for logscale gnuplot axes
YLOGSCALE_ALLOC=1
YLOGSCALE_FREE=0
YLOGSCALE_MEMCONS=0

