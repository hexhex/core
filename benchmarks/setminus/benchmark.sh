# default parameters
if [ $# -le 0 ]; then
%	confstr="--solver=dlv --flpcheck=none;--solver=genuinegc --flpcheck=none;--solver=genuinegc --extlearn --flpcheck=none"
	confstr="--solver=genuinegc --flpcheck=explicit -n=1;--solver=genuinegc --flpcheck=explicit -n=1 --extlearn"
else
	confstr=$1
fi
if [ $# -le 1 ]; then
	to=300
else
	to=$2
fi
if [ $# -le 2 ]; then
	maxsize=20
else
	maxsize=$3
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

# for all domain sizes
for (( size = 1; size <= $maxsize; size++ ))
do
	echo -ne "$size:"

	# write HEX program
	prog="nsel(X) :- domain(X), &testSetMinus[domain, sel](X)<monotonic domain,antimonotonic sel>. sel(X) :- domain(X), &testSetMinus[domain, nsel](X)<monotonic domain, antimonotonic nsel>. :- sel(X), sel(Y), sel(Z), X != Y, X != Z, Y != Z."
	for (( j = 1; j <= size; j++ ))
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

			# make sure that there are no zombie processes
			pkill -9 -u $USER dlvhex2
			pkill -9 -u $USER dlv
		else
			output="---"
		fi
		echo -ne $output
		let i=i+1
	done
	echo -e -ne "\n"
done

rm prog.hex
