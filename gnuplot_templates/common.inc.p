set style line 1 lt rgb "red"     pt 1 lw 2   #jemalloc
set style line 2 lt rgb "green"   pt 2 lw 2   # llalloc
set style line 3 lt rgb "blue"    pt 3 lw 2   # ptmalloc2
set style line 4 lt rgb "olive"   pt 4 lw 2   # tbb
set style line 5 lt rgb "cyan"    pt 5 lw 2   # tcmalloc
set style line 6 lt rgb "orange"  pt 7 lw 2   # streamflow
set style line 7 lt rgb "black"   pt 7 lw 2   # hoard

set style line 8 lt 8 pt 8 lw 2               # static
set style line 9 lt 7 pt 8 lw 2               # nulloc

set style line 40 lt rgb "red" pt 5 lw 5      # scalloc default (whatever it is)
set style line 42 lt 3 pt 7 lw 5              # scalloc-eager-madvise
set style line 45 lt rgb "orange" pt 1 lw 5   # scalloc-hugepage

#OLD
#set style line 39 lt rgb "red" pt 5 lw 5      # scalloc DEFAULT
#set style line 40 lt 1 pt 5 lw 5              # scalloc-hlab
#set style line 41 lt 1 pt 5 lw 5              # scalloc-migrate
#set style line 42 lt 3 pt 7 lw 5              # scalloc-eager-madvise
#set style line 43 lt 2 pt 8 lw 5              # scalloc-eager-reuse
#set style line 44 lt 4 pt 3 lw 5              # scalloc-lazy-init
#set style line 45 lt rgb "orange" pt 1 lw 5   # scalloc-hugepage
#set style line 46 lt 9 pt 9 lw 5              # scalloc-tlab
#set style line 47 lt 8 pt 2 lw 5              # scalloc-memory
