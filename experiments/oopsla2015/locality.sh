#!/bin/bash

# EXPERIMENT SETTINGS

#name the allocators accordingly to their .so file in $ALLOCATOR_DIR

#SCALLOCS="scalloc-core-local-1mb scalloc-tlab scalloc-eager scalloc-lazy-init scalloc-static scalloc-core-local scalloc-eager-reuse"
SCALLOCS="scalloc-clab scalloc-eager"
#OTHERS="jemalloc llalloc hoard tcmalloc ptmalloc2 streamflow tbbmalloc_proxy static"
OTHERS="jemalloc llalloc hoard tcmalloc ptmalloc2 streamflow tbbmalloc_proxy compact"

ALLOCATORS="scalloc scalloc-reuse-100"
#ALLOCATORS="$SCALLOCS $OTHERS"


OPTIONS="-a -t 1000000 -d 30 -l 1 -L 10 -s 4 -S 5 -i 1 -w 0 -A -N 2000 -C 2000 -H 4"
FACTOR1="-q"

FACTOR1_VALUES="0 10 20 30 40 50 60 70 80 90 100"
#FACTOR1_VALUES="0 20 40 60 80 100"

FACTOR2=""
FACTOR2_VALUES=""
REPS=2
#if RELATIVE is set to 1, the the response will be divided by the value for x
RELATIVE=0

TIMEOUT=1000

# OUTPUT SETTINGS
OUTPUT_DIR=`pwd`/data/oopsla2015/locality
# set to 1 for logscale gnuplot axes
#YLOGSCALE_ALLOC=0
#YLOGSCALE_ACCESS=0
#YLOGSCALE_FREE=0
#YLOGSCALE_MEMCONS=0
#YLOGSCALE_COMBINED=0

