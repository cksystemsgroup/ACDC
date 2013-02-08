#/bin/bash

OUTPUT_DIR=data/larson-threads

#runtime minsz maxsz chunks_per_thread num rounds seed max_threads
OPTIONS="2 10 1000 1000 50000"
FACTOR1=""
FACTOR2=""
REPS=2
RELATIVE=0


HEADLINE="#Created at: `date` on `hostname`"
HEADLINE="$HEADLINE\n#Average on $REPS runs. Options: $OPTIONS"
HEADLINE="$HEADLINE\n#x($FACTOR1)\tjemalloc\tstddev\tllalloc\tstddev\toptimal\tstddev\tptmalloc2\tstddev\tptmalloc3\tstddev\ttbb\tstddev\ttcmalloc\tstddev"
	
rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

echo -e $HEADLINE > $OUTPUT_DIR/runtime.dat
#echo -e $HEADLINE > $OUTPUT_DIR/free.dat
#echo -e $HEADLINE > $OUTPUT_DIR/access.dat
#echo -e $HEADLINE > $OUTPUT_DIR/memcons.dat

for XVALUE in 2 4 6 8
do
	RUNTIME_OUTPUT="$XVALUE"
	#ALLOC_OUTPUT="$XVALUE"
	#FREE_OUTPUT="$XVALUE"
	#ACCESS_OUTPUT="$XVALUE"
	#MEMCONS_OUTPUT="$XVALUE"
	for CONF in jemalloc llalloc optimal ptmalloc2 ptmalloc3 tbb tcmalloc
	do
		#skip optimal mode here
		if [ $CONF == "optimal" ]
		then
			RUNTIME_OUTPUT="$RUNTIME_OUTPUT\t0\t0"
			continue
		fi		
		#skip prmalloc3 mode here
		#if [ $CONF == "ptmalloc3" ]
		#then
		#	RUNTIME_OUTPUT="$RUNTIME_OUTPUT\t0\t0"
		#	continue
		#fi

		RUNTIME_SUM=0
		#ALLOC_SUM=0
		#FREE_SUM=0
		#ACCESS_SUM=0
		#MEMCONS_SUM=0

		for (( REP=1; REP<=$REPS; REP++ ))
		do
			#maybe derive 2nd factor from first factor?
			XVALUE2=""
			echo "./build/larson-$CONF $OPTIONS $REP $XVALUE "
			OUTPUT=$(./build/larson-$CONF $OPTIONS $XVALUE )
			
			echo $OUTPUT

			RUNTIME=$(echo "$OUTPUT" | grep Throughput)
			#MEMSTAT=$(echo "$OUTPUT" | grep MEMORY)


			read -a RUNTIME_ARRAY <<<$RUNTIME
			#read -a MEMSTAT_ARRAY <<<$MEMSTAT

			echo ${RUNTIME_ARRAY[@]}

			if [ $RELATIVE -eq 1 ]
			then
				RUNTIME_VALUE[$REP]=$(echo "scale=1; ${RUNTIME_ARRAY[1]} / $XVALUE" | bc)
				echo ${RUNTIME_VALUE[$REP]}
				#ALLOC_VALUE[$REP]=$(echo "scale=1; ${RUNTIME_ARRAY[5]} / $XVALUE" | bc)
				#FREE_VALUE[$REP]=$(echo "scale=1; ${RUNTIME_ARRAY[7]} / $XVALUE" | bc)
				#ACCESS_VALUE[$REP]=$(echo "scale=1; ${RUNTIME_ARRAY[9]} / $XVALUE" | bc)
				#MEMCONS_VALUE[$REP]=$(echo "scale=1; ${MEMSTAT_ARRAY[5]} / $XVALUE" | bc)
			else
				RUNTIME_VALUE[$REP]=${RUNTIME_ARRAY[1]}
				#ALLOC_VALUE[$REP]=${RUNTIME_ARRAY[5]}
				#FREE_VALUE[$REP]=${RUNTIME_ARRAY[7]}
				#ACCESS_VALUE[$REP]=${RUNTIME_ARRAY[9]}
				#MEMCONS_VALUE[$REP]=${MEMSTAT_ARRAY[5]}
			fi

			RUNTIME_SUM=$(echo "$RUNTIME_SUM + ${RUNTIME_VALUE[$REP]}" | bc)
			#ALLOC_SUM=$(echo "$ALLOC_SUM + ${ALLOC_VALUE[$REP]}" | bc)
			#FREE_SUM=$(echo "$FREE_SUM + ${FREE_VALUE[$REP]}" | bc)
			#ACCESS_SUM=$(echo "$ACCESS_SUM + ${ACCESS_VALUE[$REP]}" | bc)
			#MEMCONS_SUM=$(echo "$MEMCONS_SUM + ${MEMCONS_VALUE[$REP]}" | bc)
		done
		RUNTIME_AVG=$(echo "scale=1;$RUNTIME_SUM / $REPS" | bc)
		#ALLOC_AVG=$(echo "scale=1;$ALLOC_SUM / $REPS" | bc)
		#FREE_AVG=$(echo "scale=1;$FREE_SUM / $REPS" | bc)
		#ACCESS_AVG=$(echo "scale=1;$ACCESS_SUM / $REPS" | bc)
		#MEMCONS_AVG=$(echo "scale=1;$MEMCONS_SUM / $REPS" | bc)
		
		RUNTIME_SSD=0
		#ALLOC_SSD=0
		#FREE_SSD=0
		#ACCESS_SSD=0
		#MEMCONS_SSD=0
		for (( REP=1; REP<=$REPS; REP++ ))
		do
			RUNTIME_SSD=$(echo "$RUNTIME_SSD + (${RUNTIME_VALUE[$REP]} - $RUNTIME_AVG)^2" | bc)
			#ALLOC_SSD=$(echo "$ALLOC_SSD + (${ALLOC_VALUE[$REP]} - $ALLOC_AVG)^2" | bc)
			#FREE_SSD=$(echo "$FREE_SSD + (${FREE_VALUE[$REP]} - $FREE_AVG)^2" | bc)
			#ACCESS_SSD=$(echo "$ACCESS_SSD + (${ACCESS_VALUE[$REP]} - $ACCESS_AVG)^2" | bc)
			#MEMCONS_SSD=$(echo "$MEMCONS_SSD + (${MEMCONS_VALUE[$REP]} - $MEMCONS_AVG)^2" | bc)
		done
		RUNTIME_SSD=$(echo "scale=1;sqrt($RUNTIME_SSD * (1 / ($REPS - 1)))" | bc)
		#ALLOC_SSD=$(echo "scale=1;sqrt($ALLOC_SSD * (1 / ($REPS - 1)))" | bc)
		#FREE_SSD=$(echo "scale=1;sqrt($FREE_SSD * (1 / ($REPS - 1)))" | bc)
		#ACCESS_SSD=$(echo "scale=1;sqrt($ACCESS_SSD * (1 / ($REPS - 1)))" | bc)
		#MEMCONS_SSD=$(echo "scale=1;sqrt($MEMCONS_SSD * (1 / ($REPS - 1)))" | bc)
		
		RUNTIME_OUTPUT="$RUNTIME_OUTPUT\t$RUNTIME_AVG\t$RUNTIME_SSD"
		#ALLOC_OUTPUT="$ALLOC_OUTPUT\t$ALLOC_AVG\t$ALLOC_SSD"
		#FREE_OUTPUT="$FREE_OUTPUT\t$FREE_AVG\t$FREE_SSD"
		#ACCESS_OUTPUT="$ACCESS_OUTPUT\t$ACCESS_AVG\t$ACCESS_SSD"
		#MEMCONS_OUTPUT="$MEMCONS_OUTPUT\t$MEMCONS_AVG\t$MEMCONS_SSD"
	done	

	echo -e $RUNTIME_OUTPUT >> $OUTPUT_DIR/runtime.dat
	#echo -e $ALLOC_OUTPUT >> $OUTPUT_DIR/alloc.dat
	#echo -e $FREE_OUTPUT >> $OUTPUT_DIR/free.dat
	#echo -e $ACCESS_OUTPUT >> $OUTPUT_DIR/access.dat
	#echo -e $MEMCONS_OUTPUT >> $OUTPUT_DIR/memcons.dat

done

CWD=`pwd`
cp ../gnuplot_templates/*.p $OUTPUT_DIR/
cd $OUTPUT_DIR
gnuplot plot_runtime.p && epstopdf runtime.eps
#gnuplot plot_alloc.p && epstopdf alloc.eps
#gnuplot plot_free.p && epstopdf free.eps
#gnuplot plot_access.p && epstopdf access.eps
#gnuplot plot_memcons.p && epstopdf memcons.eps
cd $CWD
