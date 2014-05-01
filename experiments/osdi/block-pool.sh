#!/bin/bash

# EXPERIMENT SETTINGS

#name the allocators accordingly to their .so file in $ALLOCATOR_DIR


#scalloc-core-local is already done
#llalloc is also done

#SCALLOCS="scalloc-eager scalloc-lazy-init scalloc-static scalloc-eager-reuse"
SCALLOCS="scalloc-hugepage"
#SCALLOCS="scalloc-core-local-1mb scalloc-tlab"
#OTHERS="jemalloc hoard tcmalloc ptmalloc2 streamflow tbbmalloc_proxy"
#OTHERS="streamflow tbbmalloc_proxy"

ALLOCATORS="$SCALLOCS"

#OPTIONS="-a -s 4 -S 20 -d 30 -l 1 -L 5 -t 10000000 -O -T 5 -R 5 -N 5000 -C 500 -H 5000"
OPTIONS="-a -s 4 -S 14 -d 30 -l 1 -L 5 -t 1000000 -O -T 5 -R 5 -N 12000 -C 500 -H 14000"
FACTOR1="-n"
FACTOR1_VALUES="1 2 4 8 16 32 40 64 80 128 256 512 1024 2048 4096 8192 16384"
#FACTOR1_VALUES="1 2 4 8 16 32 64 128 256 512 1024 2048 4096"
#FACTOR1_VALUES="1 2 4 8 16 32 64 128"
#FACTOR1_VALUES="1 2 4 8 16 32 64"
#FACTOR1_VALUES="16384"
FACTOR2=""
FACTOR2_VALUES=""
REPS=5
#if RELATIVE is set to 1, the the response will be divided by the value for x
RELATIVE=1

TIMEOUT=1800

# OUTPUT SETTINGS
OUTPUT_DIR=`pwd`/data/block-pool-paperready-BI8
# set to 1 for logscale gnuplot axes
YLOGSCALE_ALLOC=1
YLOGSCALE_ACCESS=1
YLOGSCALE_FREE=1
YLOGSCALE_MEMCONS=0
YLOGSCALE_COMBINED=1

