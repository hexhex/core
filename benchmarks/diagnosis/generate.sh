# $1: observations
# $2: potential observaions probability
# $3: hypotheses
# $4: maximum dependency sets per hypothesis 
# $5: hypothesis probability
# $6: dependency probability
# $7: maximum constraints
# $8: constraint probability
# $9: constraint element probability

# Example: ./generate.sh 10 50 2 10 50 10 100 50 5

# observations
potobsProp=$((32768 * $2 / 100))
for (( d=1; d <= $3; d++ ))
do
        echo -n "hyp(h$d). "
done
echo ""
for (( d=1; d <= $1; d++ ))
do
	if [[ $RANDOM -le $potobsProp ]]; then
		echo -n "potobs(o$d). "
	else
                echo -n "obs(o$d). "
	fi
done
echo ""

# requirements
propReq=$((32768 * $5 / 100))
propDep=$((32768 * $6 / 100))
propCons=$((32768 * $8 / 100))
propConsElem=$((32768 * $9 / 100))
echo -n "program(\""
for (( t=1; t<=$1; t++ ))
do
	# another requirement?
	for (( rc=1; rc <= $4; rc++ ))
	do
		if [[ $RANDOM -le $propReq ]]; then
			first=1
			for ((d=1; d<=$3; d++ ))
			do
				if [[ $RANDOM -le $propDep ]]; then
					if [[ $first == 1 ]]; then
						echo -n "tobs(o$t) :- "
						first=0
					else
						echo -n ", "
					fi
					echo -n "hyp(h$d)"
				fi
			done
			for ((d=1; d<=$3; d++ ))
			do
				if [[ $RANDOM -le $propDep ]]; then 
					if [[ $first == 1 ]]; then
						echo -n "tobs(o$t) :- "
						first=0
					else
						echo -n ", "
					fi
					echo -n "not hyp(h$d)"
				fi
			done
			if [[ $first == 0 ]]; then
				echo -n ". "
			fi
		fi
	done
done
echo "\")."

# another constraint?
for (( rc=1; rc <= $7; rc++ ))
do
	if [[ $RANDOM -le $propCons ]]; then
		first=1
		for ((t=1; t<=$3; t++ ))
		do
                        if [[ $RANDOM -le 16384 ]]; then
                                naf="0"
                        else
                                naf="2"
                        fi
                        #if [[ $RANDOM -le 10934 ]]; then
                        #        naf="0"
                        #elif [[ $RANDOM -le 10923 ]]; then
                        #        naf="1"
                        #else
                        #        naf="2"
                        #fi
			if [[ $RANDOM -le $propConsElem ]]; then
				if [[ $first == 1 ]]; then
					echo -n ":- diagnoses(h$t, $naf)"
					first=0
				else
					echo -n ", diagnoses(h$t, $naf)"
				fi
                                #if [[ $first == 1 ]]; then
                                #        echo -n ":- not diagnoses(h$t, 2)"
                                #        first=0
                                #else
                                #        echo -n ", not diagnoses(h$t, 2)"
                                #fi
			fi
		done
		if [[ $first == 0 ]]; then
			echo ". "
		fi
	fi
done

