#!/usr/bin/awk -f
BEGIN{
	if (ARGC<2) {
		print "Usage: zeros.awk -v VAR=VALUE"
		print "Variables:"
		print "\tcol -- column, which mean square error is applied to (default: 2)"
		print "\txmin, xmax -- X-axis range (default: -inf,+inf)"
		exit 0
	}
	col =  col ? col+0 : 2
	xmin = xmin ? xmin+0. : -1e400
	xmax = xmax ? xmax+0. : +1e400
	yprev = 0
}

function abs(v) {
	return v < 0 ? -v : v
}

{
	xcurr = $1
	ycurr = $col

	if (xcurr>xmin && xcurr<xmax) {
		if (yprev < 0 && ycurr > 0 ||
			yprev > 0 && ycurr < 0) {
			z = xcurr*abs(yprev) + xprev*abs(ycurr)
			z = z / ( abs(yprev) + abs(ycurr) )
			print z
		}
		else if (ycurr == 0) {
			print xcurr
		}
	}
	xprev = xcurr
	yprev = ycurr
}
