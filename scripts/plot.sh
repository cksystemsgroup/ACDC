#!/bin/bash

if [ ! -f $1 ]; then
	echo "Experiment definition '$1' not found."
	echo "  Usage: ./scripts/plot.sh your_experiment.sh"
	exit
fi
echo "Plotting experiment $1"

source $1

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

if [ ! -n "$COMBINED_TEMPLATE" ]; then
	COMBINED_TEMPLATE="plot_memcons.p"
	if [ $YLOGSCALE_COMBINED -eq 1 ]; then
		COMBINED_TEMPLATE="plot_combined_logscale.p"
	fi
fi

CWD=`pwd`
cp -f gnuplot_templates/$ALLOC_TEMPLATE $OUTPUT_DIR/plot_alloc.p
cp -f gnuplot_templates/$FREE_TEMPLATE $OUTPUT_DIR/plot_free.p
cp -f gnuplot_templates/$MEMCONS_TEMPLATE $OUTPUT_DIR/plot_memcons.p
cp -f gnuplot_templates/$COMBINED_TEMPLATE $OUTPUT_DIR/plot_combined.p
cp -f gnuplot_templates/common.inc.p $OUTPUT_DIR
cp -f gnuplot_templates/Makefile $OUTPUT_DIR
cd $OUTPUT_DIR
make
cd $CWD
