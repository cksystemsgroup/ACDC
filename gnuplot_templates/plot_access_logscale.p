unset title

set terminal postscript eps color
set output 'access.eps'

set xlabel "x value"
set ylabel "per-thread total access time in seconds\n(lower is better)"

set logscale y
set multiplot

set key outside center bottom horizontal

plot "jemalloc-access.dat" using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 1  lw 3 title 'jemalloc', \
"llalloc-access.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 2  lw 3 title 'llalloc', \
"static-access.dat"        using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 3  lw 3 title 'static', \
"ptmalloc2-access.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 4  lw 3 title 'ptmalloc2', \
"ptmalloc3-access.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 8  lw 3 title 'ptmalloc3', \
"tbbmalloc_proxy-access.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 9  lw 3 title 'tbb', \
"tcmalloc-access.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 7  lw 3 title 'tcmalloc', \
"streamflow-access.dat"    using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 10 lw 3 title 'streamflow', \
"hoard-access.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 11 lw 3 title 'hoard', \
"scalloc-access.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 12 lw 3 title 'scalloc', \
"scalloc-eager-access.dat" using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 5 lw 3 title 'scalloc-e, \
"scalloc-core-local-access.dat" using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 11 lw 3 title 'scalloc-cl'

