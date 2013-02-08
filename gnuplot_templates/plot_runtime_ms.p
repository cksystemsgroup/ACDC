unset title


set terminal postscript eps color
set output 'runtime.eps'

set xlabel "x value"
set ylabel "Run time in ms\n(lower is better)"

set multiplot

set key outside center bottom horizontal

plot "runtime.dat" using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 1 lw 3 title 'jemalloc', \
"runtime.dat" using 1:(($4)/1):($5/1):xticlabel(1) with errorlines    lt 2 lw 3 title 'llalloc', \
"runtime.dat" using 1:(($6)/1):($7/1):xticlabel(1) with errorlines   lt 3 lw 3 title 'optimal', \
"runtime.dat" using 1:(($8)/1):($9/1):xticlabel(1) with errorlines   lt 4 lw 3 title 'ptmalloc2', \
"runtime.dat" using 1:(($10)/1):($11/1):xticlabel(1) with errorlines   lt 8 lw 3 title 'ptmalloc3', \
"runtime.dat" using 1:(($12)/1):($13/1):xticlabel(1) with errorlines   lt 9 lw 3 title 'tbb', \
"runtime.dat" using 1:(($14)/1):($15/1):xticlabel(1) with errorlines   lt 7 lw 3 title 'tcmalloc'

