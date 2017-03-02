# $1: locations
# $2: regions
# $3: adjacency propability
# $4: tag domain size
# $5: maximum dependency sets per tag
# $6: tag probability
# $7: dependency probability
# $8: maximum constraints
# $9: constraint probability
# $10: constraint element probability

# Example: ./generate.sh 10 3 10 5 10 50 10 100 50 5

# domain
for (( d=1; d <= $1; d++ ))
do
    echo -n "location(l$d). "
done
echo ""
for (( d=1; d <= $2; d++ ))
do
    echo -n "region(r$d). assign$d(L) :- assign(L, r$d). property$d(P) :- &mapping[assign$d, complexity](P)."
done
echo ""

# edges
prop=$((32768 * $3 / 100))
for (( i=1; i <= $1; i++ ))
do
    for (( j=1; j <= $1; j++ ))
    do
        if [[ $RANDOM -le $prop ]]; then
            echo -n "adjacent(l$i,l$j). "
        fi
    done
done

# requirements
propReq=$((32768 * $6 / 100))
propDep=$((32768 * $7 / 100))
propCons=$((32768 * $8 / 100))
propConsElem=$((32768 * $9 / 100))
for (( t=1; t<=$4; t++ ))
do
	# another requirement?
	for (( rc=1; rc <= $5; rc++ ))
	do
		if [[ $RANDOM -le $propReq ]]; then
			first=1
			for ((d=1; d<=$1; d++ ))
			do
				if [[ $RANDOM -le $propDep ]]; then
					if [[ $first == 1 ]]; then
						echo -n "complexity(prop$t,p"
						first=0
					fi
					echo -n ",l$d"
				fi
			done
			if [[ $first == 0 ]]; then
				echo -n ",n"
			fi
			for ((d=1; d<=$1; d++ ))
			do
				if [[ $RANDOM -le $propDep ]]; then 
					if [[ $first == 1 ]]; then
						echo -n "complexity(prop$t,p,n"
						first=0
					fi
					echo -n ",l$d"
				fi
			done
			if [[ $first == 0 ]]; then
				echo "). "
			fi
		fi
	done
done

# another constraint?
for ((d=1; d<=$1; d++ ))
do
	for (( rc=1; rc <= $7; rc++ ))
	do
		if [[ $RANDOM -le $propCons ]]; then
			first=1
			for ((t=1; t<=$4; t++ ))
			do
				if [[ $RANDOM -le $propConsElem ]]; then
					if [[ $first == 1 ]]; then
						echo -n ":-$naf property$d(prop$t)"
						first=0
					else
						echo -n ",$naf property$d(prop$t)"
					fi
				fi
			done
			if [[ $first == 0 ]]; then
				echo ". "
			fi
		fi
	done
done

