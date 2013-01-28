unset title

set terminal postscript eps color
set output 'access.eps'

set xlabel "x value"
set ylabel "access time in 10^9 cycles\n(lower is better)"

set multiplot

set key outside center bottom horizontal

plot "access.dat" using 1:($2/1000000000):($3/1000000000):xticlabel(1) with errorlines lt 1 lw 3 title 'jemalloc', \
"access.dat" using 1:(($6)/1000000000):($7/1000000000):xticlabel(1) with errorlines    lt 2 lw 3 title 'llalloc', \
"access.dat" using 1:(($10)/1000000000):($11/1000000000):xticlabel(1) with errorlines   lt 3 lw 3 title 'optimal', \
"access.dat" using 1:(($14)/1000000000):($15/1000000000):xticlabel(1) with errorlines   lt 4 lw 3 title 'ptmalloc2', \
"access.dat" using 1:(($18)/1000000000):($19/1000000000):xticlabel(1) with errorlines   lt 8 lw 3 title 'ptmalloc3', \
"access.dat" using 1:(($22)/1000000000):($23/1000000000):xticlabel(1) with errorlines   lt 9 lw 3 title 'tbb', \
"access.dat" using 1:(($26)/1000000000):($27/1000000000):xticlabel(1) with errorlines   lt 7 lw 3 title 'tcmalloc'

