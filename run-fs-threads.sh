#/bin/bash

OPTIONS="-f -s 2 -S 2 -d 50 -i 1000000"
#OPTIONS="-f -s 2 -S 2 -d 1 -i 200"

echo -e "threads\toptimal\tptmalloc\ttcmalloc\tjemalloc\ttbb"
for THREADS in 1 2 3 4 5 6 7 8
do
	OUTPUT="$THREADS"
	for CONF in optimal ptmalloc tcmalloc jemalloc tbb
	do
		T_SUM=0
		for REP in 1 2 3 4 5
		do
			RESULT=$(./build/acdc-$CONF $OPTIONS -r $REP -n $THREADS | grep RUNTIME)
			read -a ARRAY <<<$RESULT
			T=${ARRAY[9]}
			T_PER_THR=$(echo "$T/$THREADS" | bc)
			T_SUM=$(echo "${T_SUM}+${T_PER_THR}" | bc)
		done
		T_AVG=$(echo "${T_SUM}/5" | bc)
		OUTPUT="$OUTPUT\t$T_AVG"
	done
	echo -e $OUTPUT

done

