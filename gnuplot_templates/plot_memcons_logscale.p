load "common.inc.p"

unset title

set terminal postscript eps color
set output 'memcons.eps'

set xlabel "x value"
set ylabel "per-thread average memory consumption in kB\n(lower is better)" offset 1,0

set logscale y
set multiplot

#set key outside center bottom horizontal
set key off

plot \
"jemalloc-memcons.dat"      using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 1  title 'jemalloc', \
"llalloc-memcons.dat"       using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 2  title 'llalloc', \
"static-memcons.dat"        using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 8  title 'static', \
"ptmalloc2-memcons.dat"     using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 3  title 'ptmalloc2', \
"ptmalloc3-memcons.dat"     using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 8  title 'ptmalloc3', \
"tbbmalloc_proxy-memcons.dat"     using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 4  title 'tbb', \
"tcmalloc-memcons.dat"      using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 5  title 'tcmalloc', \
"streamflow-memcons.dat"    using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 6 title 'streamflow', \
"hoard-memcons.dat"         using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 7 title 'hoard', \
"scalloc-memcons.dat"       using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 9 title 'scalloc', \
"scalloc-eager-memcons.dat" using 1:($2/1024):($3/1024):xticlabel(1) with errorlines ls 10  title 'scalloc-e' \
;

