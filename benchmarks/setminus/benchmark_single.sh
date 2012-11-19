# $1: instance
# $2: confstr
# $3: timeout

# default parameters
if [ $# -le 2 ]; then
	echo "Error: invalid parameters"
	exit 1
else
	instance=$1
	confstr=$2
	to=$3
fi

# split configurations
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

# do benchmark
echo -ne "$instance "

# write HEX program
prog="
	nsel(X) :- domain(X), &testSetMinus[domain, sel](X)<monotonic domain,antimonotonic sel>.
	sel(X) :- domain(X), &testSetMinus[domain, nsel](X)<monotonic domain, antimonotonic nsel>.
	:- sel(X), sel(Y), sel(Z), X != Y, X != Z, Y != Z."
for (( j = 1; j <= instance; j++ ))
do
	prog="domain($j). $prog"
done
echo $prog > prog.hex

# for all configurations
i=0
for c in "${confs[@]}"
do
	echo -ne -e " "
	# if a configuration timed out, then it can be skipped for larger instances
	if [ ${timeout[$i]} -eq 0 ]; then
		output=$(timeout $to time -f %e dlvhex2 $c --plugindir=../../testsuite/ prog.hex 2>&1 >/dev/null)
		if [[ $? == 124 ]]; then
			output="---"
			timeout[$i]=1
		fi
	else
		output="---"
	fi
	echo -ne $output
	let i=i+1
done
echo -e -ne "\n"

rm prog.hex
