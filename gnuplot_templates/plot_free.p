unset title

set terminal postscript eps color
set output 'free.eps'

set xlabel "x value"
set ylabel "per-thread total deallocation time seconds\n(lower is better)"

set multiplot
set yrange [0:]

set key outside center bottom horizontal

plot "jemalloc-free.dat" using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 1  lw 3 title 'jemalloc', \
"llalloc-free.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 2  lw 3 title 'llalloc', \
"static-free.dat"        using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 3  lw 3 title 'static', \
"ptmalloc2-free.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 4  lw 3 title 'ptmalloc2', \
"ptmalloc3-free.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 8  lw 3 title 'ptmalloc3', \
"tbbmalloc_proxy-free.dat"     using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 9  lw 3 title 'tbb', \
"tcmalloc-free.dat"      using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 7  lw 3 title 'tcmalloc', \
"streamflow-free.dat"    using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 10 lw 3 title 'streamflow', \
"hoard-free.dat"         using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 11 lw 3 title 'hoard', \
"scalloc-free.dat"       using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 12 lw 3 title 'scalloc', \
"scalloc-eager-free.dat" using 1:($2/2000000000):($3/2000000000):xticlabel(1) with errorlines lt 5  lw 3 title 'scalloc-e'

