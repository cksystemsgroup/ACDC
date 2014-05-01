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
"scalloc-eager-access.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 10  title 'scalloc-eager-madvise', \
"scalloc-core-local-access.dat" using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 11  title 'scalloc-core-local', \
"scalloc-eager-reuse-access.dat" using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 12  title 'scalloc-eager-reuse', \
"scalloc-static-access.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 13  title 'scalloc-static-assign', \
"scalloc-lazy-init-access.dat"  using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 14  title 'scalloc-lazy-init', \
"scalloc-hugepage-access.dat"   using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 15  title 'scalloc-hugepage', \
"scalloc-access.dat"            using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 9 title 'scalloc-cl' \
"scalloc-active-threads-access.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 16 title 'scalloc-active-threads' \
;

