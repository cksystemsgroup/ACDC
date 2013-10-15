#/bin/bash

OUTPUT_DIR=data/contention-shared-threads-noaccess-mediumsize
#OPTIONS="-a -s 3 -S 10 -d 100 -l 1 -L 5 -k -t 1000000 -N 10000 -C 10000 -O -T 100 -R 100 -H 50000"
OPTIONS="-a -s 4 -S 11 -d 50 -l 1 -L 5 -t 1000000 -N 40000 -C 40000 -O -T 100 -R 100 -H 500000 -A"
FACTOR1="-n"
FACTOR2=""
REPS=2
RELATIVE=1

HEADLINE="#Created at: `date` on `hostname`"
HEADLINE="$HEADLINE\n#Average on $REPS runs. ACDC Options: $OPTIONS"
HEADLINE="$HEADLINE\n#x($FACTOR1)\tjemalloc\tstddev\tllalloc\tstddev\toptimal\tstddev\tptmalloc2\tstddev\tptmalloc3\tstddev\ttbb\tstddev\ttcmalloc\tstddev\tstreamflow\tstddev\thoard\tstddev\tscalloc\tstddev"
	
rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

echo -e $HEADLINE > $OUTPUT_DIR/alloc.dat
echo -e $HEADLINE > $OUTPUT_DIR/free.dat
echo -e $HEADLINE > $OUTPUT_DIR/access.dat
echo -e $HEADLINE > $OUTPUT_DIR/memcons.dat

for XVALUE in 1 4 8 10 20 40
do
	ALLOC_OUTPUT="$XVALUE"
	FREE_OUTPUT="$XVALUE"
	ACCESS_OUTPUT="$XVALUE"
	MEMCONS_OUTPUT="$XVALUE"
	for CONF in jemalloc llalloc optimal ptmalloc2 ptmalloc3 tbb tcmalloc streamflow hoard scalloc
	do

		ALLOC_SUM=0
		FREE_SUM=0
		ACCESS_SUM=0
		MEMCONS_SUM=0

		if [ $CONF == "dummy" \
			-o $CONF == "jemalloc" \
			-o $CONF == "llalloc" \
			-o $CONF == "optimal" \
			-o $CONF == "ptmalloc2" \
			-o $CONF == "tbb" \
			-o $CONF == "tcmalloc" \
			-o $CONF == "streamflow" \
			-o $CONF == "hoard" \
			-o $CONF == "ptmalloc3" ]
		then
			echo "skipping $CONF..."
			RUNTIME_OUTPUT="$RUNTIME_OUTPUT\t0\t0"p		
			ALLOC_OUTPUT="$ALLOC_OUTPUT\t0\t0"
			FREE_OUTPUT="$FREE_OUTPUT\t0\t0"
			ACCESS_OUTPUT="$ACCESS_OUTPUT\t0\t0"
			MEMCONS_OUTPUT="$MEMCONS_OUTPUT\t0\t0"
			continue
		fi

		if [ $CONF == "hoard" ]
		then
			export LD_PRELOAD=/home/maigner/workspace/acdc/allocators/libhoard.so
		elif [ $CONF == "streamflow" ]
		then
			export LD_PRELOAD=/home/maigner/workspace/acdc/allocators/libstreamflow.so
		#elif [ $CONF == "scalloc" ]
		#then
		#	export LD_PRELOAD=/home/mlippautz/work/scalloc/.libs/libscalloc.so
		else
			unset LD_PRELOAD
		fi

		for (( REP=1; REP<=$REPS; REP++ ))
		do
			#maybe derive 2nd factor from first factor?
			XVALUE2=""
			echo "./build/acdc-$CONF $OPTIONS -r $REP $FACTOR1 $XVALUE $FACTOR2 $XVALUE2"
			OUTPUT=$(./build/acdc-$CONF $OPTIONS -r $REP $FACTOR1 $XVALUE $FACTOR2 $XVALUE2)

			RUNTIME=$(echo "$OUTPUT" | grep RUNTIME)
			MEMSTAT=$(echo "$OUTPUT" | grep MEMORY)

			#echo $RUNTIME
			#echo $MEMSTAT

			read -a RUNTIME_ARRAY <<<$RUNTIME
			read -a MEMSTAT_ARRAY <<<$MEMSTAT

			if [ $RELATIVE -eq 1 ]
			then
				ALLOC_VALUE[$REP]=$(echo "scale=1; ${RUNTIME_ARRAY[5]} / $XVALUE" | bc)
				FREE_VALUE[$REP]=$(echo "scale=1; ${RUNTIME_ARRAY[7]} / $XVALUE" | bc)
				ACCESS_VALUE[$REP]=$(echo "scale=1; ${RUNTIME_ARRAY[9]} / $XVALUE" | bc)
				MEMCONS_VALUE[$REP]=$(echo "scale=1; ${MEMSTAT_ARRAY[5]} / $XVALUE" | bc)
			else
				ALLOC_VALUE[$REP]=${RUNTIME_ARRAY[5]}
				FREE_VALUE[$REP]=${RUNTIME_ARRAY[7]}
				ACCESS_VALUE[$REP]=${RUNTIME_ARRAY[9]}
				MEMCONS_VALUE[$REP]=${MEMSTAT_ARRAY[5]}
			fi

			ALLOC_SUM=$(echo "$ALLOC_SUM + ${ALLOC_VALUE[$REP]}" | bc)
			FREE_SUM=$(echo "$FREE_SUM + ${FREE_VALUE[$REP]}" | bc)
			ACCESS_SUM=$(echo "$ACCESS_SUM + ${ACCESS_VALUE[$REP]}" | bc)
			MEMCONS_SUM=$(echo "$MEMCONS_SUM + ${MEMCONS_VALUE[$REP]}" | bc)
		done
		ALLOC_AVG=$(echo "scale=1;$ALLOC_SUM / $REPS" | bc)
		FREE_AVG=$(echo "scale=1;$FREE_SUM / $REPS" | bc)
		ACCESS_AVG=$(echo "scale=1;$ACCESS_SUM / $REPS" | bc)
		MEMCONS_AVG=$(echo "scale=1;$MEMCONS_SUM / $REPS" | bc)
		
		ALLOC_SSD=0
		FREE_SSD=0
		ACCESS_SSD=0
		MEMCONS_SSD=0
		for (( REP=1; REP<=$REPS; REP++ ))
		do
			ALLOC_SSD=$(echo "$ALLOC_SSD + (${ALLOC_VALUE[$REP]} - $ALLOC_AVG)^2" | bc)
			FREE_SSD=$(echo "$FREE_SSD + (${FREE_VALUE[$REP]} - $FREE_AVG)^2" | bc)
			ACCESS_SSD=$(echo "$ACCESS_SSD + (${ACCESS_VALUE[$REP]} - $ACCESS_AVG)^2" | bc)
			MEMCONS_SSD=$(echo "$MEMCONS_SSD + (${MEMCONS_VALUE[$REP]} - $MEMCONS_AVG)^2" | bc)
		done
		ALLOC_SSD=$(echo "scale=1;sqrt($ALLOC_SSD * (1 / ($REPS - 1)))" | bc)
		FREE_SSD=$(echo "scale=1;sqrt($FREE_SSD * (1 / ($REPS - 1)))" | bc)
		ACCESS_SSD=$(echo "scale=1;sqrt($ACCESS_SSD * (1 / ($REPS - 1)))" | bc)
		MEMCONS_SSD=$(echo "scale=1;sqrt($MEMCONS_SSD * (1 / ($REPS - 1)))" | bc)
		
		ALLOC_OUTPUT="$ALLOC_OUTPUT\t$ALLOC_AVG\t$ALLOC_SSD"
		FREE_OUTPUT="$FREE_OUTPUT\t$FREE_AVG\t$FREE_SSD"
		ACCESS_OUTPUT="$ACCESS_OUTPUT\t$ACCESS_AVG\t$ACCESS_SSD"
		MEMCONS_OUTPUT="$MEMCONS_OUTPUT\t$MEMCONS_AVG\t$MEMCONS_SSD"
	done	

	echo -e $ALLOC_OUTPUT >> $OUTPUT_DIR/alloc.dat
	echo -e $FREE_OUTPUT >> $OUTPUT_DIR/free.dat
	echo -e $ACCESS_OUTPUT >> $OUTPUT_DIR/access.dat
	echo -e $MEMCONS_OUTPUT >> $OUTPUT_DIR/memcons.dat

done

CWD=`pwd`
cp gnuplot_templates/*.p $OUTPUT_DIR/
cd $OUTPUT_DIR
gnuplot plot_alloc.p && epstopdf alloc.eps
gnuplot plot_free.p && epstopdf free.eps
gnuplot plot_access.p && epstopdf access.eps
gnuplot plot_memcons.p && epstopdf memcons.eps
cd $CWD
