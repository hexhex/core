#!/bin/bash

source dlvhex_run_header.sh

# run instances
if [[ $all -eq 1 ]]; then
	# run all instances using the benchmark script run insts
	$bmscripts/runinsts.sh "{1..20}" "$mydir/run.sh" "$mydir" "$to"
else
	# run single instance
	confstr="--flpcheck=explicit --noflpcriterion;--flpcheck=explicit --extlearn --noflpcriterion;--flpcheck=ufsm --noflpcriterion;--flpcheck=ufsm --extlearn --noflpcriterion;--flpcheck=ufsm --extlearn --ufslearn --noflpcriterion;--flpcheck=ufs;--flpcheck=ufs --extlearn;--flpcheck=ufs --extlearn --ufslearn;--flpcheck=aufs;--flpcheck=aufs --extlearn;--flpcheck=aufs --extlearn --ufslearn;--flpcheck=explicit --noflpcriterion -n=1;--flpcheck=explicit --extlearn --noflpcriterion -n=1;--flpcheck=ufsm -n=1;--flpcheck=ufsm --extlearn --noflpcriterion -n=1;--flpcheck=ufsm --extlearn --ufslearn --noflpcriterion -n=1;--flpcheck=ufs -n=1;--flpcheck=ufs --extlearn -n=1;--flpcheck=ufs --extlearn --ufslearn -n=1;--flpcheck=aufs -n=1;--flpcheck=aufs --extlearn -n=1;--flpcheck=aufs --extlearn --ufslearn -n=1"

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


	$bmscripts/runconfigs.sh "dlvhex2 --plugindir=../../testsuite CONF prog$instance.hex" "$confstr" "$instance" "$to"
	rm prog$instance.hex
fi

