#/bin/bash

#OPTIONS="-a -s 3 -S 10 -l 1 -L 10 -t 100000000 -i 0 -d 50 -O -n 8 -T 100"
OPTIONS="-a -s 3 -S 20 -l 1 -L 1 -t 100000000 -i 0 -d 20 -O -n 4 -T 100"
#OPTIONS="-a -s 3 -S 10 -l 1 -L 10 -t 1000000 -i 0 -d 20 -O -n 8 -T 100"
REPS=5
echo "#Created at: `date` on `hostname`"
echo "#Average on $REPS runs. ACDC Options: $OPTIONS"

echo -e "#share-ratio\tjemalloc\tstddev\tllalloc\tstddev\toptimal\tstddev\tptmalloc2\tstddev\tptmalloc3\tstddev\ttbb\tstddev\ttcmalloc\tstddev"
#for THREADS in 1 2 3 4 5 6 7 8
for XVALUE in {0..100..10}
do
	OUTPUT="$XVALUE"
	for CONF in jemalloc llalloc optimal ptmalloc2 ptmalloc3 tbb tcmalloc
	do
		VALUE_SUM=0
		for (( REP=1; REP<=$REPS; REP++ ))
		do
			RESULT=$(./build/acdc-$CONF $OPTIONS -r $REP -R $XVALUE | grep MEMORY)
			read -a ARRAY <<<$RESULT
			#VALUE[$REP]=$(echo "scale=1; ${ARRAY[5]} / $XVALUE" | bc)
			VALUE[$REP]=${ARRAY[5]}
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

