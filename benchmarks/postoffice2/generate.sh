# $1: locations
# $2: regions
# $3: maximum distance
# $4: maximum region size
# $5: maximum allowed distance

# Example: ./generate.sh 10 3 10 3 5

# domain
for (( d=1; d <= $1; d++ ))
do
    echo -n "location(l$d). "
done
echo ""
for (( d=1; d <= $2; d++ ))
do
    echo -n "region(r$d). maxSize(r$d, $4). maxDist(r$d, $5)."
done
echo ""

# distances
for (( i=1; i <= $1; i++ ))
do
    for (( j=1; j <= $2; j++ ))
    do
	dist=$(($RANDOM * $3 / 32768))
        echo -n "distance(l$i,r$j,$dist). "
    done
done
