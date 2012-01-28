if [ $# -le 3 ]; then
	echo "Usage:"
	echo "     benchmark.sh [Program] [Min Domain Size] [Max Domain Size] [Timeout/s] [Configuration Strings (optional)]"
	echo "The configuration string consists of dlvhex options with semicolon as delimiter"
	exit 0
fi

# $5: configurations
if [ $# -le 4 ]; then
	confstr=";--internalsolver;--internalsolver --extlearn"
else
	confstr=$5
fi
IFS=';' read -ra confs <<< "$confstr"
header="#size"
i=0
for c in "${confs[@]}"
do
	timeout[$i]=0
	header="$header   \"$c\""
	let i=i+1
done
echo $header


# $1: program
# $2: min domain size
# $3: max domain size
# $4: timeout in seconds

for (( size = $2 ; size <= $3 ; size++ ))
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

	line="$size"
	i=0
	for c in "${confs[@]}"
	do
		if [ ${timeout[$i]} -eq 0 ]; then
			/usr/bin/time -o time.txt -f %e dlvhex $c p.hex 2>/dev/null >/dev/null
			output=`cat time.txt`
			timeout[$i]=`echo "$output > $4" | bc`
			if [ ${timeout[$i]} -eq 1 ]; then
				output=$4
			fi
			line="$line   $output"
		else
			line="$line   $4"
		fi
		let i=i+1
	done
	echo $line
done
