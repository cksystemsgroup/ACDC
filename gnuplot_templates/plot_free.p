load "common.inc.p"

unset title

set terminal postscript eps color
set output 'free.eps'

set xlabel "number of threads"
set ylabel "per-thread total deallocation time seconds\n(lower is better)" offset 2,0

set multiplot
set yrange [0:]

set key outside center bottom horizontal
#set key off

plot \
"jemalloc-free.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 1  title 'jemalloc', \
"llalloc-free.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 2  title 'llalloc', \
"static-free.dat"        using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'static', \
"ptmalloc2-free.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 3  title 'ptmalloc2', \
"ptmalloc3-free.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'ptmalloc3', \
"tbbmalloc_proxy-free.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 4  title 'tbb', \
"tcmalloc-free.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 5  title 'tcmalloc', \
"streamflow-free.dat"    using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 6 title 'streamflow', \
"hoard-free.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 7 title 'hoard', \
"nulloc-free.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 9 title 'nulloc', \
"scalloc-hlab-free.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 10  title 'scalloc-hlab', \
"scalloc-tlab-free.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 11  title 'scalloc-tlab', \
"scalloc-eager-free.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 12  title 'scalloc-eager', \
"scalloc-hugepage-free.dat"   using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 13  title 'scalloc-hugepage' \
;
