#/bin/bash

ALLOCATOR_DIR=`pwd`/allocators
#name the allocators accordingly to their .so file
ALLOCATORS="jemalloc llalloc static ptmalloc2 ptmalloc3 tbbmalloc tcmalloc streamflow hoard scalloc"
OPTIONS="-a -s 3 -S 9 -d 50 -l 1 -L 5 -t 1000000 -N 40000 -C 40000 -H 500000"
FACTOR1="-n"
FACTOR1_VALUES="1 4 8 12 16 20 40 60 80"
FACTOR2=""
FACTOR2_VALUES=""
REPS=3
#if RELATIVE is set to 1, the the respoinse will be divided by the value for x
RELATIVE=1

OUTPUT_DIR=data/contention-threads-noaccess

