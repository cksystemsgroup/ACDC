#!/bin/bash

#name the allocators accordingly to their .so file
ALLOCATORS="jemalloc llalloc ptmalloc2 tbbmalloc_proxy tcmalloc streamflow hoard scalloc scalloc-eager"
OPTIONS="-a -s 4 -S 20 -d 20 -l 1 -L 5 -t 10000000 -N 40000 -C 40000 -H 500000"
FACTOR1="-n"
FACTOR1_VALUES="1 2 4 8 10 20 40 60 80"
FACTOR2=""
FACTOR2_VALUES=""
REPS=5
#if RELATIVE is set to 1, the the respoinse will be divided by the value for x
RELATIVE=1

OUTPUT_DIR=data/contention-threads-noaccess-mediumsize
# set to 1 for logscale gnuplot axes
YLOGSCALE_ALLOC=1
YLOGSCALE_FREE=1
YLOGSCALE_MEMCONS=0


