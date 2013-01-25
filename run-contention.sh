#/bin/bash

OPTIONS="-a -s 2 -S 10 -l 1 -L 10 -i 0 -t 100000000 -d 10"
REPS=5
echo "#Created at: `date` on `hostname`"
echo "#Average on $REPS runs. ACDC Options: $OPTIONS"

echo -e "#x-value\tjemalloc_alloc\tstddev\tjemalloc_free\tstddev\tllalloc_alloc\tstddev\tllalloc_free\tstddev\toptimal_alloc\tstddev\toptimal_free\tstddev\tptmalloc2_alloc\tstddev\tptmalloc2_free\tstddev\tptmalloc3_alloc\tstddev\tptmalloc3_free\tstddev\ttbb_alloc\tstddev\ttbb_free\tstddev\ttcmalloc_alloc\tstddev\ttcmalloc_free\tstddev"

for XVALUE in 1 2 4 6 8 12 16 20 24
do
	OUTPUT="$XVALUE"
	for CONF in jemalloc llalloc optimal ptmalloc2 ptmalloc3 tbb tcmalloc
	do
		VALUE1_SUM=0
		VALUE2_SUM=0
		for (( REP=1; REP<=$REPS; REP++ ))
		do
			RESULT=$(./build/acdc-$CONF $OPTIONS -r $REP -n $XVALUE | grep RUNTIME)
			read -a ARRAY <<<$RESULT

			# VALUE1 is total allocation time
			VALUE1[$REP]=$(echo "scale=1; ${ARRAY[5]} / $XVALUE" | bc)
			#echo ${VALUE1[$REP]}
			# VALUE 2 is total deallocation time
			VALUE2[$REP]=$(echo "scale=1; ${ARRAY[7]} / $XVALUE" | bc)
			#echo ${VALUE2[$REP]}
			VALUE1_SUM=$(echo "$VALUE1_SUM + ${VALUE1[$REP]}" | bc)
			VALUE2_SUM=$(echo "$VALUE2_SUM + ${VALUE2[$REP]}" | bc)
		done
		AVG1=$(echo "scale=1;$VALUE1_SUM / $REPS" | bc)
		AVG2=$(echo "scale=1;$VALUE2_SUM / $REPS" | bc)
		SSD1=0
		SSD2=0
		for (( REP=1; REP<=$REPS; REP++ ))
		do
			SSD1=$(echo "$SSD1 + (${VALUE1[$REP]} - $AVG1)^2"|bc)
			SSD2=$(echo "$SSD2 + (${VALUE2[$REP]} - $AVG2)^2"|bc)
		done
		SSD1=$(echo "scale=1;sqrt($SSD1 * (1 / ($REPS - 1)))"|bc)
		SSD2=$(echo "scale=1;sqrt($SSD2 * (1 / ($REPS - 1)))"|bc)
		OUTPUT="$OUTPUT\t$AVG1\t$SSD1\t$AVG2\t$SSD2"
	done
	echo -e $OUTPUT
done

