# $1: instance
# $2: timeout

# default parameters
export PATH=$1
export LD_LIBRARY_PATH=$2
instance=$3
to=$4

confstr="--flpcheck=explicit;--flpcheck=explicit --extlearn;--flpcheck=ufsm --noflpcriterion;--flpcheck=ufsm --extlearn --noflpcriterion;--flpcheck=ufsm --extlearn --ufslearn --noflpcriterion;--flpcheck=ufs;--flpcheck=ufs --extlearn;--flpcheck=ufs --extlearn --ufslearn;--flpcheck=aufs;--flpcheck=aufs --extlearn;--flpcheck=aufs --extlearn --ufslearn;--flpcheck=explicit -n=1;--flpcheck=explicit --extlearn -n=1;--flpcheck=ufsm -n=1;--flpcheck=ufsm --extlearn --noflpcriterion -n=1;--flpcheck=ufsm --extlearn --ufslearn --noflpcriterion -n=1;--flpcheck=ufs -n=1;--flpcheck=ufs --extlearn -n=1;--flpcheck=ufs --extlearn --ufslearn -n=1;--flpcheck=aufs -n=1;--flpcheck=aufs --extlearn -n=1;--flpcheck=aufs --extlearn --ufslearn -n=1"

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


# for all configurations
i=0
for c in "${confs[@]}"
do
	echo -ne -e " "
	output=$(timeout $to time -o time$instance.dat -f %e dlvhex2 $c --plugindir=../../testsuite/ --verbose=0 prog$instance.hex 2>/dev/null >/dev/null)
	ret=$?
	if [[ $ret == 0 ]]; then
	        output=$(cat time$instance.dat)
	else
		output="---"
	fi
	echo -ne "$output"
	let i=i+1
done
echo -e -ne "\n"

rm prog$instance.hex
rm time$instance.dat
