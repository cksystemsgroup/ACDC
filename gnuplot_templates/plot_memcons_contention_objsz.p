load "common.inc.p"

unset title

set terminal postscript eps color
set output 'memcons.eps'

set xlabel "object size in bytes (logscale)"
set ylabel "average memory consumption in MB\n(logscale, lower is better)" offset 1,0

set multiplot
set logscale y

set xtics 0,2,25
set xtics add ("16-64B" 4)
set xtics add ("64-256B" 6)
set xtics add ("256-1KB" 8)
set xtics add ("1-4KB" 10)
set xtics add ("4-16KB" 12)
set xtics add ("16-64KB" 14)
set xtics add ("64-256KB" 16)
set xtics add ("256KB-1MB" 18)
set xtics add ("1-4MB" 20)
set xtics add ("4-16MB" 22)
set xtics add ("16-64MB" 24)
set xtics add ("32-128MB" 25)

set xtics rotate by -90

#set key outside center bottom horizontal
set key off

plot \
"jemalloc-memcons.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 1  title 'jemalloc', \
"llalloc-memcons.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 2  title 'llalloc', \
"static-memcons.dat"        using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'static', \
"ptmalloc2-memcons.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 3  title 'ptmalloc2', \
"ptmalloc3-memcons.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 8  title 'ptmalloc3', \
"tbbmalloc_proxy-memcons.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 4  title 'tbb', \
"tcmalloc-memcons.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 5  title 'tcmalloc', \
"streamflow-memcons.dat"    using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 6 title 'streamflow', \
"hoard-memcons.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 7 title 'hoard', \
"scalloc-memcons.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 9 title 'scalloc', \
"scalloc-eager-memcons.dat" using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines ls 10  title 'scalloc-e' \
;

