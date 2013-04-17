# $1: instance
# $2: timeout

# default parameters
export PATH=$1
export LD_LIBRARY_PATH=$2
instance=$3
to=$4

confstr="--extlearn --flpcheck=aufs ../reachability_strongsafety.hex $instance;--extlearn --flpcheck=aufs --liberalsafety ../reachability.hex $instance"

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
echo -ne "$instance"

# for all configurations
i=0
for c in "${confs[@]}"
do
	echo -ne -e " "
	output=$(timeout $to time -o $instance.time.dat -f %e dlvhex2 $c --plugindir=../../../testsuite/ --verbose=8 2>$instance.verbose.dat >/dev/null)
	ret=$?
        output=$(cat $instance.time.dat)
	groundertime=$(cat $instance.verbose.dat | grep -a "HEX grounder time:" | tail -n 1 | grep -P -o '[0-9]+\.[0-9]+s' | sed "s/s//")
        solvertime=$(cat $instance.verbose.dat | grep -a "HEX solver time:" | tail -n 1 | grep -P -o '[0-9]+\.[0-9]+s' | sed "s/s//")

	if [[ $ret == 124 ]]; then
		output="---"
		groundertime="---"
		solvertime="---"
	fi
	echo -ne "$output $groundertime $solvertime"
	let i=i+1
done
echo -e -ne "\n"

rm $instance.time.dat
#rm $instance.verbose.dat
