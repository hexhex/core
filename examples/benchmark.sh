if [ $# -le 3 ]; then
	echo "Usage:"
	echo "     benchmark.sh [Program] [Min Domain Size] [Max Domain Size] [Timeout/s] [Configuration Strings (optional)]"
	echo "The configuration string consists of dlvhex options with semicolon as delimiter"
	exit 0
fi

# $5: configurations
if [ $# -le 4 ]; then
	confstr=";--genuinesolver;--genuinesolver --extlearn"
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


# create working directory
wd=`mktemp -d $PWD/tmp.XXXXXXXXXX`
extension=$(echo $1 | sed "s/.*\.\([a-z|A-Z|0-9]*$\)/\1/" | tr "[:upper:]" "[:lower:]")


# $1: program
# $2: min domain size
# $3: max domain size
# $4: timeout in seconds

for (( size = $2 ; size <= $3 ; size++ ))
do
	if [ "$extension" = "hex" ]; then
		# construct domain
		domain=""
		for (( i = 1 ; i <= $size ; i++ ))
		do
			domain="$domain domain($i)."
		done

		# construct program
		cat $1 > $wd/prog.hex
		echo $domain >> $wd/prog.hex
	fi
	if [ "$extension" = "sh" ]; then
		# call shell script to construct program
		./$1 $wd $size
	fi

	line="$size"
	i=0
	for c in "${confs[@]}"
	do
		if [ ${timeout[$i]} -eq 0 ]; then
			/usr/bin/time -o time.txt -f %e dlvhex $c $wd/prog.hex 2>/dev/null >/dev/null
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

# cleanup
rm -r $wd
