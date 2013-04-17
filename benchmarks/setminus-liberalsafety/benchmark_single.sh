# $1: instance
# $2: timeout

# default parameters
export PATH=$1
export LD_LIBRARY_PATH=$2
instance=$3
to=$4

confstr="--extlearn --flpcheck=aufs prog$instance.hex;--extlearn --flpcheck=aufs --liberalsafety prognd$instance.hex"

# split configurations
IFS=';' read -ra confs <<< "$confstr"
header="#size"
i=0
for c in "${confs[@]}"
do
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
echo $prog > prog$instance.hex

prog="
        nsel(X) :- &testSetMinus[domain, sel](X)<monotonic domain,antimonotonic sel,relativefinitedomain 0 domain>.
        sel(X) :- &testSetMinus[domain, nsel](X)<monotonic domain, antimonotonic nsel,relativefinitedomain 0 domain>.
        :- sel(X), sel(Y), sel(Z), X != Y, X != Z, Y != Z."
for (( j = 1; j <= instance; j++ ))
do
        prog="domain($j). $prog"
done
echo $prog > prognd$instance.hex


# for all configurations
i=0
for c in "${confs[@]}"
do
	echo -ne -e " "
	output=$(timeout $to time -o time$instance.dat -f %e dlvhex2 $c --plugindir=../../testsuite/ --verbose=8 2>verbose$instance.dat >/dev/null)
	ret=$?
        output=$(cat time$instance.dat)
	groundertime=$(cat verbose$instance.dat | grep -a "HEX grounder time:" | tail -n 1 | grep -P -o '[0-9]+\.[0-9]+s' | sed "s/s//")
        solvertime=$(cat verbose$instance.dat | grep -a "HEX solver time:" | tail -n 1 | grep -P -o '[0-9]+\.[0-9]+s' | sed "s/s//")

	if [[ $ret == 124 ]]; then
		output="---"
		groundertime="---"
		solvertime="---"
	fi
	echo -ne $output $groundertime $solvertime
	let i=i+1
done
echo -e -ne "\n"

rm prog$instance.hex
rm prognd$instance.hex
rm time$instance.dat
rm verbose$instance.dat
