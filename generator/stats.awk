{ times[NR] = $1 }
END {
    if ( NR > 0 ) {
	if (NR < 3) {
	    n = NR
	    b = 1
	    e = NR
	} else {
	    n = NR - 2
	    b = 2
	    e = NR - 1
	}
	    
	for (i = b; i <= e; i++) {
	    sum += times[i]
	}
	
	avg = sum/n
	
	if ( n % 2 ) {
	    med = times[int(n/2) + 1]
	} else {
	    med = (times[n/2] + times[n/2 + 1]) / 2
	}
	
	for (i = b; i <= e; i++) {
	    sumtimes += (times[i] - avg) * (times[i] - avg)
	}
	
	dvn = sqrt(sumtimes/n)
	if ( n == 1 ) {
		sdvn = 0	
	} else {
		sdvn = sqrt(sumtimes/(n-1))
	}
    }
    
    # print "cnt: ", n, " sum: ", sum, " avg: ", avg, " med: ", med, " dvn: ", dvn, " sdvn: ", sdvn
    printf "cnt: %d  sum: %4.2f  avg: %4.2f  med: %4.2f  dvn: %4.2f  sdvn: %4.2f\n", n, sum, avg, med, dvn, sdvn
}
