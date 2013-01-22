#/bin/bash


OPTIONS="-f -s 2 -S 2 -l 1 -L 1 -t 1 -d 100 -n $1"

./build/acdc-optimal  $OPTIONS 
./build/acdc-ptmalloc $OPTIONS 
./build/acdc-tcmalloc $OPTIONS 
./build/acdc-jemalloc $OPTIONS 
./build/acdc-tbb      $OPTIONS 


