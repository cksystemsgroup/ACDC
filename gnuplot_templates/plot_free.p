unset title


set terminal postscript eps color
set output 'free.eps'

set xlabel "x value"
set ylabel "total deallocation time per thread in milliseconds\n(lower is better)"

set multiplot

set key outside center bottom horizontal

plot "free.dat" using 1:($2/2100000):($3/2100000):xticlabel(1) with errorlines lt 1 lw 3 title 'jemalloc', \
"free.dat" using 1:(($4)/2100000):($5/2100000):xticlabel(1) with errorlines    lt 2 lw 3 title 'llalloc', \
"free.dat" using 1:(($6)/2100000):($7/2100000):xticlabel(1) with errorlines   lt 3 lw 3 title 'static', \
"free.dat" using 1:(($8)/2100000):($9/2100000):xticlabel(1) with errorlines   lt 4 lw 3 title 'ptmalloc2', \
"free.dat" using 1:(($10)/2100000):($11/2100000):xticlabel(1) with errorlines   lt 8 lw 3 title 'ptmalloc3', \
"free.dat" using 1:(($12)/2100000):($13/2100000):xticlabel(1) with errorlines   lt 9 lw 3 title 'tbb', \
"free.dat" using 1:(($14)/2100000):($15/2100000):xticlabel(1) with errorlines   lt 7 lw 3 title 'tcmalloc', \
"free.dat" using 1:(($14)/2100000):($15/2100000):xticlabel(1) with errorlines   lt 10 lw 3 title 'scalloc'

