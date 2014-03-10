#/bin/bash

#name the allocators accordingly to their .so file
ALLOCATORS="jemalloc llalloc ptmalloc2 tbbmalloc_proxy tcmalloc streamflow hoard scalloc scalloc-eager static"
#ALLOCATORS="static"
OPTIONS="-a -t 500000 -d 50 -l 1 -L 10 -s 4 -S 5 -i 1 -w 0 -A -N 10000 -C 10000 -H 4000"
FACTOR1="-q"
FACTOR1_VALUES="0 10 20 30 40 50 60 70 80 90 100"
REPS=5
#if RELATIVE is set to 1, the the respoinse will be divided by the value for x
RELATIVE=0

OUTPUT_DIR=data/locality-lists-vs-trees

#overriding plot_templates for this experiment only
ACCESS_TEMPLATE="plot_access_locality.p"

