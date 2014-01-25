# $1: instance size

# random number 
p=$RANDOM

for (( i=1; i <= $1; i++ ))
do
	# write something to the set of facts
	echo "someFact($i)."

	# write something to the Abox
	echo "<owl some Abox assertion for individual \"$1\"/>" 1>&2
done
