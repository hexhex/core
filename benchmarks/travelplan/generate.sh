# $1: node count
# $2: edge propability
# $3: probability that an edge (a,b) is forbidden

echo "#maxint=$1."

prop=$((32768 * $2 / 100)) 
fprop=$((32768 * $2 / 100))
for (( i=1; i <= $1; i++ ))
do
	for (( j = 1; j <= $1; j++ ))
	do
		if [[ $i != $j ]] && [ $RANDOM -le $prop ]; then
			echo "edge($i,$j,1)."
			if [ $RANDOM -le $fprop ]; then
				echo ":- anyST($i,$j)."
			fi
		fi
	done
done

echo "location(X) :- edge(X, Y, C). location(Y) :- edge(X, Y, C)."
