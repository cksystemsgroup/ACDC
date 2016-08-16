#!/bin/bash

# EXPERIMENT SETTINGS

#name the allocators accordingly to their .so file

SCALLOCS="scalloc"
OTHERS="hoard tcmalloc ptmalloc2 streamflow tbbmalloc_proxy llalloc jemalloc"
#OTHERS="tbbmalloc_proxy llalloc jemalloc"

#ALLOCATORS="compact $OTHERS scalloc-clab scalloc-eager"
#ALLOCATORS="$SCALLOCS $OTHERS"
ALLOCATORS="llalloc"

# original settings for oopsla paper 
OPTIONS="-a -d 5 -l 1 -L 1 -n 40 -N 1000 -C 500 -H 100 -A"


FACTOR1="-s"
FACTOR1_VALUES="4 6 8 10 12 14 16 18 20"
#FACTOR1_VALUES="4 6 8 10 12 14"
#FACTOR1_VALUES="20"

FACTOR2="-S"
FACTOR2_EXPRESSION="X + 2" # X will be replaced by factor1's value

FACTOR3="-t"
FACTOR3_EXPRESSION="2^X * 1024" # X will be replaced by factor1's value

#REPS=5
REPS=3
#if RELATIVE is set to 1, the the respoinse will be divided by the value for x
RELATIVE=0

# OUTPUT SETTINGS
OUTPUT_DIR=`pwd`/data/oopsla2015/virtual-spans
#overriding plot_templates for this experiment only
#ALLOC_TEMPLATE="plot_alloc_contention_objsz.p"
#ACCESS_TEMPLATE="plot_access_contention_objsz.p"
#FREE_TEMPLATE="plot_free_contention_objsz.p"
#MEMCONS_TEMPLATE="plot_memcons_contention_objsz.p"

#YLOGSCALE_ALLOC=1
#YLOGSCALE_FREE=1
#YLOGSCALE_MEMCONS=1
#YLOGSCALE_COMBINED=1

