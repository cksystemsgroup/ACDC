#/bin/bash


OPTIONS="-f -s 2 -S 2 -d 1000 -i 1000000 -n $1"

./build/acdc-optimal  $OPTIONS 
./build/acdc-ptmalloc $OPTIONS 
./build/acdc-tcmalloc $OPTIONS 
./build/acdc-jemalloc $OPTIONS 
./build/acdc-tbb      $OPTIONS 


