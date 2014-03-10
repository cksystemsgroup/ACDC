#/bin/bash


if [ ! -f $1 ]; then
	echo "Experiment definition '$1' not found."
	echo "  Usage: ./scripts/run-acdc.sh your_experiment.sh"
	exit
fi
echo "Running Experiment $1"

source $1


ALLOCATOR_DIR=`pwd`/allocators
ACDC=`pwd`/out/Release/acdc

if [ ! -d $ALLOCATOR_DIR ]; then
	echo "Cannot find directory containing the allocators"
	echo "try ./install_allocators.sh and run scripts from ACDC root dir"
	exit
fi

export LD_LIBRARY_PATH=$ALLOCATOR_DIR

HEADLINE="#Created at: `date` on `hostname`"
HEADLINE="$HEADLINE\n#Average on $REPS runs. ACDC Options: $OPTIONS"
HEADLINE="$HEADLINE\n#x($FACTOR1)\taverage\tstddev"

#rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

for ALLOCATOR in $ALLOCATORS
do
	#echo "running ACDC config for $ALLOCATOR"

	echo -e $HEADLINE > $OUTPUT_DIR/$ALLOCATOR-alloc.dat
	echo -e $HEADLINE > $OUTPUT_DIR/$ALLOCATOR-free.dat
	echo -e $HEADLINE > $OUTPUT_DIR/$ALLOCATOR-access.dat
	echo -e $HEADLINE > $OUTPUT_DIR/$ALLOCATOR-memcons.dat
	ALLOC_OUTPUT=""
	FREE_OUTPUT=""
	ACCESS_OUTPUT=""
	MEMCONS_OUTPUT=""

	for XVALUE in $FACTOR1_VALUES
	do	
		ALLOC_OUTPUT="$ALLOC_OUTPUT\n$XVALUE"
		FREE_OUTPUT="$FREE_OUTPUT\n$XVALUE"
		ACCESS_OUTPUT="$ACCESS_OUTPUT\n$XVALUE"
		MEMCONS_OUTPUT="$MEMCONS_OUTPUT\n$XVALUE"
		ALLOC_SUM=0
		FREE_SUM=0
		ACCESS_SUM=0
		MEMCONS_SUM=0

		for (( REP=1; REP<=$REPS; REP++ ))
		do
			COMMAND="$ACDC -P $ALLOCATOR $OPTIONS -r $REP $FACTOR1 $XVALUE"
			
			#maybe derive 2nd factor from first factor?
			if [ -n $FACTOR2 ]; then
				#echo "Processing expression for factor 2: $FACTOR2_EXPRESSION"
				EXP1=${FACTOR2_EXPRESSION/X/$XVALUE}
				#echo $EXP1
				XVALUE2=$(echo "$EXP1" | bc);
				#echo $XVALUE2
				COMMAND="$COMMAND $FACTOR2 $XVALUE2"
			fi

			#maybe derive 3rd factor from first factor?
			if [ -n $FACTOR3 ]; then
				#echo "Processing expression for factor 3: $FACTOR3_EXPRESSION"
				EXP2=${FACTOR3_EXPRESSION/X/$XVALUE}
				#echo "$EXP2"
				XVALUE3=$(echo "$EXP2" | bc);
				#echo $XVALUE3
				COMMAND="$COMMAND $FACTOR3 $XVALUE3"
			fi

			echo "Running: $COMMAND"
			
			#ptmalloc2 requires no LD_PRELOAD. everything else does
			unset LD_PRELOAD
			if [ $ALLOCATOR != "ptmalloc2" -a $ALLOCATOR != "static" ]; then
				export LD_PRELOAD=$ALLOCATOR_DIR/lib$ALLOCATOR.so
			fi
			#OUTPUT=$($ACDC -P $ALLOCATOR $OPTIONS -r $REP $FACTOR1 $XVALUE $FACTOR2 $XVALUE2)
			OUTPUT=$($COMMAND)
			unset LD_PRELOAD

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
		done #REPS
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
	done #XVALUE
	echo -e $ALLOC_OUTPUT >> $OUTPUT_DIR/$ALLOCATOR-alloc.dat
	echo -e $FREE_OUTPUT >> $OUTPUT_DIR/$ALLOCATOR-free.dat
	echo -e $ACCESS_OUTPUT >> $OUTPUT_DIR/$ALLOCATOR-access.dat
	echo -e $MEMCONS_OUTPUT >> $OUTPUT_DIR/$ALLOCATOR-memcons.dat
done #ALLOCATORS

if [ ! -n "$ALLOC_TEMPLATE" ]; then
	ALLOC_TEMPLATE="plot_alloc.p"
	if [ $YLOGSCALE_ALLOC -eq 1 ]; then
		ALLOC_TEMPLATE="plot_alloc_logscale.p"
	fi
fi

if [ ! -n "$ACCESS_TEMPLATE" ]; then
	ACCESS_TEMPLATE="plot_access.p"
	if [ $YLOGSCALE_ACCESS -eq 1 ]; then
		ACCESS_TEMPLATE="plot_access_logscale.p"
	fi
fi

if [ ! -n "$FREE_TEMPLATE" ]; then
	FREE_TEMPLATE="plot_free.p"
	if [ $YLOGSCALE_FREE -eq 1 ]; then
		FREE_TEMPLATE="plot_free_logscale.p"
	fi
fi

if [ ! -n "$MEMCONS_TEMPLATE" ]; then
	MEMCONS_TEMPLATE="plot_memcons.p"
	if [ $YLOGSCALE_MEMCONS -eq 1 ]; then
		MEMCONS_TEMPLATE="plot_memcons_logscale.p"
	fi
fi

CWD=`pwd`
cp -f gnuplot_templates/$ALLOC_TEMPLATE $OUTPUT_DIR/plot_alloc.p
cp -f gnuplot_templates/$FREE_TEMPLATE $OUTPUT_DIR/plot_free.p
cp -f gnuplot_templates/$MEMCONS_TEMPLATE $OUTPUT_DIR/plot_memcons.p
cp -f gnuplot_templates/common.inc.p $OUTPUT_DIR
cp -f gnuplot_templates/Makefile $OUTPUT_DIR
cd $OUTPUT_DIR
make
cd $CWD
