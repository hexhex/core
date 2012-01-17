# $1: program
# $2: min domain size
# $3: max domain size
# $4: timeout in seconds

echo "#size     dlv     genuine     genuine+extlearn"

dlvto=0
gento=0
genlearnto=0

for (( size = $2 ; size < $3 ; size++ ))
do

	# construct domain
	domain=""
	for (( i = 1 ; i <= $size ; i++ ))
	do
		domain="$domain domain($i)."
	done

	# construct program
	cat $1 > p.hex
	echo $domain >> p.hex

	# call dlvhex with translation to dlv
	if [ $dlvto -eq 0 ]; then
		dlv=`/usr/bin/time -f %e dlvhex p.hex 2>&1 >/dev/null`
		dlvto=`echo "$dlv > $4" | bc`
		if [ $dlvto -eq 1 ]; then
			dlv=$4
		fi
	fi

	# call dlvhex with genuine solver without learning
	if [ $gento -eq 0 ]; then
		gen=`/usr/bin/time -f %e dlvhex --internalsolver p.hex 2>&1 >/dev/null`
		gento=`echo "$gen > $4" | bc`
		if [ $gento -eq 1 ]; then
			gen=$4
		fi
	fi

	# call dlvhex with genuine solver with learning
	if [ $genlearnto -eq 0 ]; then
		genlearn=`/usr/bin/time -f %e dlvhex --internalsolver --extlearn p.hex 2>&1 >/dev/null`
		genlearnto=`echo "$genlearn > $4" | bc`
		if [ $genlearnto -eq 1 ]; then
			genlearn=$4
		fi
	fi

	echo "$size         $dlv    $gen        $genlearn"

done
