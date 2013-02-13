#/bin/bash

OUTPUT_DIR=data/lever-boreham-emulation
#OPTIONS="-a -s 9 -S 9 -t 1 -l 1 -L 1 -k -d 90000 -g 1000000 -F 100"
OPTIONS="-a -s 9 -S 9 -t 1 -l 1 -L 1 -k -d 100000 -g 1000000 -F 100"
FACTOR1="-n"
FACTOR2=""
REPS=5
RELATIVE=1

HEADLINE="#Created at: `date` on `hostname`"
HEADLINE="$HEADLINE\n#Average on $REPS runs. ACDC Options: $OPTIONS"
HEADLINE="$HEADLINE\n#x($FACTOR1)\tjemalloc\tstddev\tllalloc\tstddev\toptimal\tstddev\tptmalloc2\tstddev\tptmalloc3\tstddev\ttbb\tstddev\ttcmalloc\tstddev"
	
rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

echo -e $HEADLINE > $OUTPUT_DIR/runtime.dat
#echo -e $HEADLINE > $OUTPUT_DIR/alloc.dat
#echo -e $HEADLINE > $OUTPUT_DIR/free.dat
#echo -e $HEADLINE > $OUTPUT_DIR/access.dat
#echo -e $HEADLINE > $OUTPUT_DIR/memcons.dat

for XVALUE in 1 2 4 6 8 10 12 16 20 24
do
	echo $XVALUE
	RUNTIME_OUTPUT="$XVALUE"
	for CONF in jemalloc llalloc optimal ptmalloc2 ptmalloc3 tbb tcmalloc
	do
		echo $CONF
		#skip optimal mode here
		if [ $CONF == "optimal" ]
		then
			RUNTIME_OUTPUT="$RUNTIME_OUTPUT\t0\t0"
			continue
		fi		
		#skip prmalloc3 mode here
		#if [ $CONF == "tbb" ]
		#then
		#	RUNTIME_OUTPUT="$RUNTIME_OUTPUT\t0\t0"
		#	continue
		#fi
		
		RUNTIME_SUM=0

		for (( REP=1; REP<=$REPS; REP++ ))
		do
			#maybe derive 2nd factor from first factor?
			XVALUE2=""
			OUTPUT=$(./build/acdc-$CONF $OPTIONS -r $REP $FACTOR1 $XVALUE $FACTOR2 $XVALUE2)

			RUNTIME=$(echo "$OUTPUT" | grep RUNTIME)

			read -a RUNTIME_ARRAY <<<$RUNTIME

			RUNTIME_VALUE[$REP]=$(echo "scale=1; (${RUNTIME_ARRAY[5]} + ${RUNTIME_ARRAY[7]})" | bc)	
			if [ $RELATIVE -eq 1 ]
			then
				RUNTIME_VALUE[$REP]=$(echo "scale=1; ${RUNTIME_VALUE[REP]} / $XVALUE" | bc)
			fi

			RUNTIME_SUM=$(echo "$RUNTIME_SUM + ${RUNTIME_VALUE[$REP]}" | bc)

		done
		RUNTIME_AVG=$(echo "scale=1;$RUNTIME_SUM / $REPS" | bc)
		RUNTIME_SSD=0

		for (( REP=1; REP<=$REPS; REP++ ))
		do
			RUNTIME_SSD=$(echo "$RUNTIME_SSD + (${RUNTIME_VALUE[$REP]} - $RUNTIME_AVG)^2" | bc)			
		done

		RUNTIME_SSD=$(echo "scale=1;sqrt($RUNTIME_SSD * (1 / ($REPS - 1)))" | bc)

		RUNTIME_OUTPUT="$RUNTIME_OUTPUT\t$RUNTIME_AVG\t$RUNTIME_SSD"

	done	
	
	echo -e $RUNTIME_OUTPUT >> $OUTPUT_DIR/runtime.dat

done

CWD=`pwd`
cp gnuplot_templates/*.p $OUTPUT_DIR/
cd $OUTPUT_DIR

gnuplot plot_runtime.p && epstopdf runtime.eps

cd $CWD
