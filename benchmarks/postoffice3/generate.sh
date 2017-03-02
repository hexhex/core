# $1: number of regions
# $2: number of locations
# $3: max locations assigned to region (max size)
# $4: max allowed distance to region (max dist)
# $5: max distance to region (range of distances of locations to regions: 0..$7)

echo -n "maxsize(i$3)."
echo -n "maxdist(i$4)."

# domain
for (( d=1; d <= $2; d++ ))
do
    echo -n "location(l$d). "
done
echo ""

for (( d=1; d <= $1; d++ ))
do
    echo -n "region(r$d). "
done
echo ""

# distances
for (( i=1; i <= $2; i++ ))
do
    for (( j=1; j <= $1; j++ ))
    do
	dist=$(($RANDOM * $5 / 32768))
        echo -n "distance(l$i,r$j,i$dist). "
    done
done

# unequal
for (( i=1; i <= $1; i++ ))
do
    for (( j=1; j <= $1; j++ ))
    do
	if [[ $i != $j ]]; then
		echo -n "unequal(r$i,r$j). "
	fi
    done
done
