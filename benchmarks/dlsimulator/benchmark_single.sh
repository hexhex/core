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
                #maxint=$instance.
                birds(X) :- #int(X).
                flies(X) :- birds(X), not neg_flies(X).

                % Single out the non-fliers under default assumption:
                pcflier(X) :- flies(X).
                neg_flies(X) :- birds(X), &testDLSimulator[1, $instance, pcflier](X)<fullylinear>.

                % Is the description logic KB inconsistent?
                :- inconsistent.
                inconsistent :- not &testDLSimulator[0, $instance, pcflier](0)." > prog$instance.hex

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
                #maxint=$instance.
                birds(X) :- #int(X).
                flies(X) :- birds(X), not neg_flies(X).

                % Single out the non-fliers under default assumption:
                pcflier(X) :- flies(X).
                neg_flies(X) :- &testDLSimulator[1, $instance, pcflier](X)<fullylinear, finitedomain 0>.

                % Is the description logic KB inconsistent?
                :- inconsistent.
                inconsistent :- not &testDLSimulator[0, $instance, pcflier](0)." > prognd$instance.hex


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
	echo -ne "$output $groundertime $solvertime "
	let i=i+1
done
echo -e -ne "\n"

rm prog$instance.hex
rm prognd$instance.hex
rm time$instance.dat
rm verbose$instance.dat
