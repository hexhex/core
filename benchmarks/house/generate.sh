# $1: node count
# $2: edge propability

echo "#maxint=$1."

prop=$((32768 * $2 / 100)) 
for (( i=1; i <= $1; i++ ))
do
	echo "person($i)."
        echo "thing($i)."
        echo "cabinet($i)."
        echo "room($i)."
	for (( j = 1; j <= $1; j++ ))
	do
		if [ $RANDOM -le $prop ]; then
			echo "personTOthing($i,$j)."
		fi
	done
done
