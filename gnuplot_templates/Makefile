all:
	gnuplot plot_alloc.p
	gnuplot plot_free.p
	gnuplot plot_access.p
	gnuplot plot_memcons.p
	epstopdf alloc.eps
	epstopdf free.eps
	epstopdf access.eps
	epstopdf memcons.eps

clean:
	rm -rf alloc.pdf free.pdf access.pdf memcons.pdf
	rm -rf alloc.eps free.eps access.eps memcons.eps
