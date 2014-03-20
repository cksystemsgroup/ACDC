#!/bin/bash

# This is an example for an ACDC experiment definition

# EXPERIMENT SETTINGS

# ALLOCATORS
# list for all allocators under test. Experiments will be run for each allocator
# name the allocators accordingly to their .so file in the allocators directory
ALLOCATORS="jemalloc llalloc ptmalloc2 tbbmalloc_proxy tcmalloc streamflow hoard scalloc scalloc-eager static"

# OPTIONS
# ACDC options that are fixed for all runs in this experiment definition
OPTIONS="-a -d 5 -l 1 -L 1 -n 40 -N 100000 -C 100000 -H 1000000 -A"

# FACTOR1
# the changing factor depicted on the x axis
FACTOR1="-s"
FACTOR1_VALUES="4 6 8 10 12 14 16 18 20"

# FACTOR2
# optional: a second fanctor might change in some relation to FACTOR1
# The relation is given in FACTOR2_EXPRESSION
FACTOR2="-S"

# FACTOR2_EXPRESSION
FACTOR2_EXPRESSION="X + 2" # X will be replaced by factor1's value

# FACTOR3
# works like FACTOR2
FACTOR3="-t"
FACTOR3_EXPRESSION="2^X * 1024" # X will be replaced by factor1's value

# REPS
# Number of replications of the experiment
REPS=5

# RELATIVE
#if RELATIVE is set to 1, the the respoinse on the y axis will be divided by the value for x
RELATIVE=0

# OUTPUT SETTINGS

# OUTPUT_DIR
# dat files and plots go here
OUTPUT_DIR=`pwd`/data/experiment_name_results

# YLOGSCALE_[metric]
# set to 1 for logscale y axes. Default 0
YLOGSCALE_ALLOC=1
YLOGSCALE_FREE=0
YLOGSCALE_MEMCONS=0

# [metric]_TEMPLATE
# specify .p file in gnuplot_templates. This everrides YLOGSCALE_[metric]
# ALLOC_TEMPLATE="plot_alloc_contention_objsz.p"
# ACCESS_TEMPLATE="plot_access_contention_objsz.p"
# FREE_TEMPLATE="plot_free_contention_objsz.p"
# MEMCONS_TEMPLATE="plot_memcons_contention_objsz.p"


