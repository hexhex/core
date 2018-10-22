# $1: node count

frac=$(((1 * 100) / $1))
prop=$(((32768 * $frac)/100))

echo $prop
for (( i=1; i <= $1; i++ ))
do
	for (( j = 1; j <= $1; j++ ))
	do
		if [ $RANDOM -le $prop ]; then
			echo "n$i"
			echo "n$j"
		fi
	done
done
