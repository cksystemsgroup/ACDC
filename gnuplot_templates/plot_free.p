unset title


set terminal postscript eps color
set output 'free.eps'

set xlabel "x value"
set ylabel "deallocation time in 10^9 cycles\n(lower is better)"

set multiplot

set key outside center bottom horizontal

plot "free.dat" using 1:($2/1000000000):($3/1000000000):xticlabel(1) with errorlines lt 1 lw 3 title 'jemalloc', \
"free.dat" using 1:(($4)/1000000000):($5/1000000000):xticlabel(1) with errorlines    lt 2 lw 3 title 'llalloc', \
"free.dat" using 1:(($6)/1000000000):($7/1000000000):xticlabel(1) with errorlines   lt 3 lw 3 title 'optimal', \
"free.dat" using 1:(($8)/1000000000):($9/1000000000):xticlabel(1) with errorlines   lt 4 lw 3 title 'ptmalloc2', \
"free.dat" using 1:(($10)/1000000000):($11/1000000000):xticlabel(1) with errorlines   lt 8 lw 3 title 'ptmalloc3', \
"free.dat" using 1:(($12)/1000000000):($13/1000000000):xticlabel(1) with errorlines   lt 9 lw 3 title 'tbb', \
"free.dat" using 1:(($14)/1000000000):($15/1000000000):xticlabel(1) with errorlines   lt 7 lw 3 title 'tcmalloc'

