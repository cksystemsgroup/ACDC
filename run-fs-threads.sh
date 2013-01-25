#/bin/bash

OPTIONS="-f -s 2 -S 4 -d 50 -i 1000000"
#OPTIONS="-f -s 2 -S 2 -d 1 -i 200"
REPS=5
echo "#Created at: `date` on `hostname`"
echo "#Average on $REPS runs. ACDC Options: $OPTIONS"
echo -e "#threads\tjemalloc\tstddev\tllalloc\tstddev\toptimal\tstddev\tptmalloc2\tstddev\tptmalloc3\tstddev\ttbb\tstddev\ttcmalloc\tstddev"

for THREADS in 1 2 4 6 8 12 16 20 24
do
	OUTPUT="$THREADS"
	for CONF in jemalloc llalloc optimal ptmalloc2 ptmalloc3 tbb tcmalloc
	do
		VALUE_SUM=0
		for (( REP=1; REP<=$REPS; REP++ ))
		do
			RESULT=$(./build/acdc-$CONF $OPTIONS -r $REP -n $THREADS | grep RUNTIME)
			read -a ARRAY <<<$RESULT
			VALUE[$REP]=$(echo "scale=1; ${ARRAY[9]} / $THREADS" | bc)
			VALUE_SUM=$(echo "$VALUE_SUM + ${VALUE[$REP]}" | bc)
		done
		AVG=$(echo "scale=1;$VALUE_SUM / $REPS" | bc)
		SSD=0
		for (( REP=1; REP<=$REPS; REP++ ))
		do
			SSD=$(echo "$SSD + (${VALUE[$REP]} - $AVG)^2"|bc)
		done
		SSD=$(echo "scale=1;sqrt($SSD * (1 / ($REPS - 1)))"|bc)
		OUTPUT="$OUTPUT\t$AVG\t$SSD"
	done
	echo -e $OUTPUT
done

