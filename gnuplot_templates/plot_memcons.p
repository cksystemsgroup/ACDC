unset title

set terminal postscript eps color
set output 'memcons.eps'

set xlabel "x value"
set ylabel "per-thread average memory consumption in kB\n(lower is better)"

set multiplot

set key outside center bottom horizontal

plot "jemalloc-memcons.dat"     using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 1  lw 3 title 'jemalloc', \
"llalloc-memcons.dat"           using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 2  lw 3 title 'llalloc', \
"static-memcons.dat"            using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 3  lw 3 title 'static', \
"ptmalloc2-memcons.dat"         using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 4  lw 3 title 'ptmalloc2', \
"ptmalloc3-memcons.dat"         using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 8  lw 3 title 'ptmalloc3', \
"tbbmalloc_proxy-memcons.dat"         using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 9  lw 3 title 'tbb', \
"tcmalloc-memcons.dat"          using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 7  lw 3 title 'tcmalloc', \
"streamflow-memcons.dat"        using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 10 lw 3 title 'streamflow', \
"hoard-memcons.dat"             using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 11 lw 3 title 'hoard', \
"scalloc-memcons.dat"           using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 12 lw 3 title 'scalloc'

