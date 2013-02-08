unset title


set terminal postscript eps color
set output 'alloc.eps'

set xlabel "x value"
set ylabel "total allocation time per thread in milliseconds\n(lower is better)"

set multiplot

set key outside center bottom horizontal

plot "alloc.dat" using 1:($2/2100000):($3/2100000):xticlabel(1) with errorlines lt 1 lw 3 title 'jemalloc', \
"alloc.dat" using 1:(($4)/2100000):($5/2100000):xticlabel(1) with errorlines    lt 2 lw 3 title 'llalloc', \
"alloc.dat" using 1:(($6)/2100000):($7/2100000):xticlabel(1) with errorlines   lt 3 lw 3 title 'static', \
"alloc.dat" using 1:(($8)/2100000):($9/2100000):xticlabel(1) with errorlines   lt 4 lw 3 title 'ptmalloc2', \
"alloc.dat" using 1:(($10)/2100000):($11/2100000):xticlabel(1) with errorlines   lt 8 lw 3 title 'ptmalloc3', \
"alloc.dat" using 1:(($12)/2100000):($13/2100000):xticlabel(1) with errorlines   lt 9 lw 3 title 'tbb', \
"alloc.dat" using 1:(($14)/2100000):($15/2100000):xticlabel(1) with errorlines   lt 7 lw 3 title 'tcmalloc'

