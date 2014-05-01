load "common.inc.p"

unset title

set terminal postscript eps color
set output 'combined.eps'

set xlabel "number of threads"
set ylabel "per-thread total time spent in allocator in seconds\n(lower is better)" offset 2,0

set multiplot
set yrange [0:]

set key outside center bottom horizontal
#set key off

plot \
"jemalloc-combined.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 1  title 'jemalloc', \
"llalloc-combined.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 2  title 'llalloc', \
"static-combined.dat"        using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'static', \
"ptmalloc2-combined.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 3  title 'ptmalloc2', \
"ptmalloc3-combined.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'ptmalloc3', \
"tbbmalloc_proxy-combined.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 4  title 'tbb', \
"tcmalloc-combined.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 5  title 'tcmalloc', \
"streamflow-combined.dat"    using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 6 title 'streamflow', \
"hoard-combined.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 7 title 'hoard', \
"nulloc-combined.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 9 title 'nulloc', \
"scalloc-eager-combined.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 10  title 'scalloc-eager-madvise', \
"scalloc-core-local-combined.dat"          using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 11  title 'scalloc-core-local', \
"scalloc-eager-reuse-combined.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 12  title 'scalloc-eager-reuse', \
"scalloc-static-combined.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 13  title 'scalloc-static-assign', \
"scalloc-lazy-init-combined.dat"           using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 14  title 'scalloc-lazy-init', \
"scalloc-hugepage-combined.dat"            using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 15  title 'scalloc-hugepage', \
"scalloc-active-threads-combined.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 16 title 'scalloc-active-threads' \
;

