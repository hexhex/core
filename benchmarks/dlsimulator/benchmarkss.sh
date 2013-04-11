# default parameters
if [ $# -le 0 ]; then
	confstr="--extlearn"
else
	confstr=$1
fi
if [ $# -le 1 ]; then
	to=300
else
	to=$2
fi
if [ $# -le 2 ]; then
	maxsize=200
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
	echo "
		%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		%
		% Tweety (1) -- The Bird Case     
		%
		% This is the formulation of the famous \"birds fly by default\" 
		% example from the non-monotonic reasoning literature where 
		% Tweety is known to be a bird.
		%
		% The OWL ontology contains the knowledge about Birds,
		% Penguins and Fliers, and that Tweety is a bird; the birds-fly-
		% by-default rule is formulated on top of the ontology by
		% nonmonotonic rules.
		% 
		% We then can query whether Tweety flies, and get the intuitive 
		% result.
		%
		% We don't use here strong negation (\"-\") on LP predicates in rules, 
		% since well-founded semantics for dl-programs is only defined-
		% in absence of "-". As for answer set semantics, just replace
		% \"neg_\" by \"-\".
		%
		%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


		% By default, a bird flies:
		#maxint=$size.
		birds(X) :- #int(X).
		flies(X) :- birds(X), not neg_flies(X).

		% Single out the non-fliers under default assumption:
		pcflier(X) :- flies(X).
		neg_flies(X) :- birds(X), &testDLSimulator[1, $size, pcflier](X)<fullylinear>.

		% Is the description logic KB inconsistent? 
		:- inconsistent.
		inconsistent :- not &testDLSimulator[0, $size, pcflier](0)." > prog.hex

	# for all configurations
	i=0
	for c in "${confs[@]}"
	do
		echo -ne -e " "
		# if a configuration timed out, then it can be skipped for larger instances
		if [ ${timeout[$i]} -eq 0 ]; then
			# run dlvhex
			output=$(timeout $to time -o time.dat -f %e dlvhex2 $c --plugindir=../../testsuite/ --verbose=8 prog.hex 2>solverstat.dat >/dev/null)

			output=$(cat time.dat)
			solvertime=$(cat solverstat.dat | grep "Solver time:" | grep -P -o '[0-9]+\.[0-9]+s' | sed "s/s//")
			groundertime=$(cat solverstat.dat | grep "Grounder time:" | grep -P -o '[0-9]+\.[0-9]+s' | sed "s/s//")
			computeExtensionOfDomainPredicatestime=$(cat solverstat.dat | grep "computeExtensionOfDomainPredicates:" | grep -P -o '[0-9]+\.[0-9]+s' | sed "s/s//")
			computeExtensionOfDomainPredicatestime="0.000"
			if [[ $? == 124 ]]; then
				output="---"
				timeout[$i]=1
			fi
		else
			output="--- --- --- ---"
		fi
		echo -ne $output $solvertime $groundertime $computeExtensionOfDomainPredicatestime
		let i=i+1
	done
	echo -e -ne "\n"
done

rm prog.hex
