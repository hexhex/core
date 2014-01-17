# $1: node count
# $2: edge propability
m=$(($1 * 3))
echo "#maxint=$m."

for (( i=1; i <= $1; i++ ))
do
	echo "person($i)."
done

for (( i=1; i <= 2 * $1; i++ ))
do
        echo "cabinet($i)."
done

for (( i=1; i <= 1 + $1 / 2; i++ ))
do
        echo "room($i)."
done

for (( i=1; i <= 3 * $1; i++ ))
do
	echo "thing($i)."
	p=$((1 + $1 * $RANDOM / 32768))
	echo "personTOthing($p,$i)."
done
