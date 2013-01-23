#/bin/bash

OPTIONS="-f -s 2 -S 2 -d 5 -i 20000000"
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
			RESULT=`./build/acdc-$CONF $OPTIONS -n $THREADS -r $REP | grep RUNTIME`
			read -a ARRAY <<<$RESULT
			T=${ARRAY[9]}
			let "T_PER_THR = $T / $THREADS"
			let "T_SUM = $T_SUM + $T_PER_THR"
		done
		let "T_AVG = T_SUM / 5"
		OUTPUT="$OUTPUT\tT_AVG"
	done
	echo -e $OUTPUT

done

