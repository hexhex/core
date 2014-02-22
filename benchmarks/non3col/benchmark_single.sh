# $1: instance
# $2: timeout

# default parameters
export PATH=$1
export LD_LIBRARY_PATH=$2
instance=$3
to=$4

confstr="checkNon3Colorability.hex;checkNon3Colorability.hex --supportsets;checkNon3ColorabilityPlain.hex;checkNon3CompleteColorability.hex;checkNon3CompleteColorability.hex --supportsets;checkNon3CompleteColorabilityPlain.hex"
confstr2=$(cat conf)
if [ $? == 0 ]; then
        confstr=$confstr2
fi

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
	pushd .. > /dev/null 2>&1
	output=$(timeout $to time -o $instance.$i.time.dat -f %e dlvhex2 $c --plugindir=../../testsuite/ instances/$instance --verbose=8 2>$instance.$i.verbose.dat > /dev/null)
	ret=$?
	if [[ $ret == 0 ]]; then
	        output=$(cat $instance.$i.time.dat)
		groundertime=$(cat $instance.$i.verbose.dat | grep -a "HEX grounder time:" | tail -n 1 | grep -P -o '[0-9]+\.[0-9]+s' | sed "s/s//")
	        solvertime=$(cat $instance.$i.verbose.dat | grep -a "HEX solver time:" | tail -n 1 | grep -P -o '[0-9]+\.[0-9]+s' | sed "s/s//")
	else
		output="---"
		groundertime="---"
		solvertime="---"
	fi
	echo -ne "$output $groundertime $solvertime"

	rm $instance.$i.time.dat
	rm $instance.$i.verbose.dat

	popd > /dev/null 2>&1
	let i=i+1
done
echo -e -ne "\n"
