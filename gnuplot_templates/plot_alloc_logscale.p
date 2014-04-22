load "common.inc.p"

unset title

set terminal postscript eps color
set output 'alloc.eps'

set xlabel "number of threads"
set ylabel "per-thread total allocation time seconds\n(logscale, lower is better)" offset 2,0

set logscale y
set multiplot

set key outside center bottom horizontal
#set key off

plot \
"jemalloc-alloc.dat"            using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 1  title 'jemalloc', \
"llalloc-alloc.dat"             using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 2  title 'llalloc', \
"static-alloc.dat"              using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'static', \
"ptmalloc2-alloc.dat"           using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 3  title 'ptmalloc2', \
"ptmalloc3-alloc.dat"           using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'ptmalloc3', \
"tbbmalloc_proxy-alloc.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 4  title 'tbb', \
"tcmalloc-alloc.dat"            using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 5  title 'tcmalloc', \
"streamflow-alloc.dat"          using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 6 title 'streamflow', \
"hoard-alloc.dat"               using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 7 title 'hoard', \
"nulloc-alloc.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 9 title 'nulloc', \
"scalloc-eager-alloc.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 10  title 'scalloc-eager-madvise', \
"scalloc-core-local-alloc.dat"          using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 11  title 'scalloc-core-local', \
"scalloc-eager-reuse-alloc.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 12  title 'scalloc-eager-reuse', \
"scalloc-static-alloc.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 13  title 'scalloc-static-assign', \
"scalloc-lazy-init-alloc.dat"           using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 14  title 'scalloc-lazy-init', \
"scalloc-hugepage-alloc.dat"            using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 15  title 'scalloc-hugepage', \
"scalloc-alloc.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 9 title 'scalloc-cl' \
;
