#/bin/bash

#OPTIONS="-a -s 3 -S 10 -d 100 -l 1 -L 5 -k -t 1000000 -N 10000 -C 10000 -O -T 100 -R 100 -H 50000"
OPTIONS="-a -s 3 -S 9 -d 50 -l 1 -L 5 -t 1000000 -N 40000 -C 40000 -O -T 100 -R 100 -H 500000"
FACTOR1="-n"
FACTOR2=""
REPS=2
RELATIVE=1

OUTPUT_DIR=data/contention-shared-threads-noaccess

