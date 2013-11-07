all:
	for i in *.p; do gnuplot $i; done
	for i in *.eps; do epstopdf $i; done
clean:
	rm -rf alloc.pdf free.pdf access.pdf memcons.pdf
	rm -rf alloc.eps free.eps access.eps memcons.eps
