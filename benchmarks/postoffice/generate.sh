# $1: locations
# $2: regions
# $3: adjacency propability
# $4: maximum region size

# Example: ./generate.sh 10 3 10 3

# domain
for (( d=1; d <= $1; d++ ))
do
    echo -n "location(l$d). "
done
echo ""
for (( d=1; d <= $2; d++ ))
do
    echo -n "region(r$d). maxSize(r$d, $4). "
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

