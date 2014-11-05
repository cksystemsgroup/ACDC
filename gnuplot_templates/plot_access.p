load "common.inc.p"
unset title

set terminal postscript eps color
set output 'access.eps'

set xlabel "x value"
set ylabel "per-thread total access time in seconds\n(lower is better)"

set multiplot
set yrange [0:]

set key outside center bottom horizontal

plot \
"jemalloc-access.dat"           using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 1  title 'jemalloc', \
"llalloc-access.dat"            using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 2  title 'llalloc', \
"static-access.dat"             using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'static', \
"ptmalloc2-access.dat"          using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 3  title 'ptmalloc2', \
"ptmalloc3-access.dat"          using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'ptmalloc3', \
"tbbmalloc_proxy-access.dat"    using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 4  title 'tbb', \
"tcmalloc-access.dat"           using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 5  title 'tcmalloc', \
"streamflow-access.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 6 title 'streamflow', \
"hoard-access.dat"              using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 7 title 'hoard', \
"nulloc-access.dat"             using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 9 title 'nulloc', \
"scalloc-clab-access.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 10  title 'scalloc-clab', \
"scalloc-tlab-access.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 11  title 'scalloc-tlab', \
"scalloc-eager-access.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 12  title 'scalloc-eager', \
"scalloc-hugepage-access.dat"   using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 13  title 'scalloc-hugepage' \
;
