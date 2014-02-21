# check input validity
inputok=1
if [[ $# -eq 0 ]]; then
	inputok=1
elif [[ $# -gt 5 ]]; then
	inputok=0
elif [[ $1 != "all" ]] && [[ $1 != "single" ]]; then
	inputok=0
fi
if [[ $inputok -eq 0 ]]; then
	echo "This script expects 0 to 5 parameters" >&2
	echo "   \$1: (optional) \"all\" or \"single\", default is \"all\"" >&2
	echo "   (a) If \$1=\"all\" then there are no further parameters" >&2
	echo "       \$2: (optional) timeout, default is 300" >&2
	echo "       \$3: (optional) directory with the benchmark scripts" >&2
	echo "   (b) If \$1=\"single\" then" >&2
	echo "       \$2: instance name" >&2
	echo "       \$3: timeout in seconds" >&2
	echo "       \$4: directory with the benchmark scripts" >&2
	echo "       \$5: (optional) timeout, default is 300" >&2
	exit 1
fi

# set default values
# and get location of benchmark scripts
if [[ $# -eq 0 ]]; then
	all=1
elif [[ $1 == "all" ]]; then
	all=1
else
	all=0
fi
if [[ $all -eq 1 ]]; then
	if [[ $# -ge 2 ]]; then
		to=$2
	else
		to=300
	fi
	if [[ $# -ge 3 ]]; then
		bmscripts=$3
	else
		runinstsdir=$(which runinsts.sh | head -n 1)
		if [ -e "$runinstsdir" ]; then
			bmscripts=$(dirname "$runinstsdir")
		fi
	fi
else
	instance=$2
	to=$3
	bmscripts=$4
fi
if ! [ -e "$bmscripts" ]; then
	echo "Could not find benchmark scripts"
	exit 1
fi

# run instances
mydir="$(dirname $0)"
if [[ $all -eq 1 ]]; then
	# run all instances using the benchmark script run insts
	$bmscripts/runinsts.sh "{1..20}" "$mydir/run.sh" "$mydir" "$to"
else
	# run single instance
	confstr="--extlearn --flpcheck=aufs prog$instance.hex;--extlearn --flpcheck=aufs --liberalsafety prognd$instance.hex"

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

	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite CONF prog$instance.hex" "$confstr" "$instance" "$to" "$bmscripts/gstimeoutputbuilder.sh"
	rm prog$instance.hex
	rm prognd$instance.hex
fi

