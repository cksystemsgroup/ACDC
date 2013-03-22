unset title

set terminal postscript eps color
set output 'memcons.eps'

set xlabel "x value"
set ylabel "memory consumption in kB\n(lower is better)"

set multiplot

set key outside center bottom horizontal

plot "memcons.dat" using 1:($2/1):($3/1):xticlabel(1) with errorlines lt 1 lw 3 title 'jemalloc', \
"memcons.dat" using 1:(($4)/1):($5/1):xticlabel(1) with errorlines    lt 2 lw 3 title 'llalloc', \
"memcons.dat" using 1:(($6)/1):($7/1):xticlabel(1) with errorlines   lt 3 lw 3 title 'static', \
"memcons.dat" using 1:(($8)/1):($9/1):xticlabel(1) with errorlines   lt 4 lw 3 title 'ptmalloc2', \
"memcons.dat" using 1:(($10)/1):($11/1):xticlabel(1) with errorlines   lt 8 lw 3 title 'ptmalloc3', \
"memcons.dat" using 1:(($12)/1):($13/1):xticlabel(1) with errorlines   lt 9 lw 3 title 'tbb', \
"memcons.dat" using 1:(($14)/1):($15/1):xticlabel(1) with errorlines   lt 7 lw 3 title 'tcmalloc', \
"memcons.dat" using 1:(($16)/1):($17/1):xticlabel(1) with errorlines   lt 10 lw 3 title 'streamflow', \
"memcons.dat" using 1:(($18)/1):($19/1):xticlabel(1) with errorlines   lt 11 lw 3 title 'hoard'

