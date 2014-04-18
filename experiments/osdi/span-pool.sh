#!/bin/bash

#name the allocators accordingly to their .so file
#ALLOCATORS="jemalloc llalloc ptmalloc2 tbbmalloc_proxy tcmalloc streamflow hoard scalloc scalloc-eager"
ALLOCATORS="jemalloc llalloc scalloc-core-local scalloc-eager-reuse scalloc-eager scalloc-lazy-init scalloc-static"
OPTIONS="-a -s 4 -S 20 -d 30 -l 1 -L 5 -t 10000000 -N 5000 -C 500 -H 5000"
FACTOR1="-n"
FACTOR1_VALUES="1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192"
FACTOR2=""
FACTOR2_VALUES=""
REPS=2
#if RELATIVE is set to 1, the the respoinse will be divided by the value for x
RELATIVE=1

OUTPUT_DIR=data/span-pool
# set to 1 for logscale gnuplot axes
YLOGSCALE_ALLOC=1
YLOGSCALE_ACCESS=1
YLOGSCALE_FREE=1
YLOGSCALE_MEMCONS=0


