# $1: size

echo -n "list(\""

for (( i=1; i <= $1; i++ ))
do
	if [ $i -ge 2 ]; then
		echo -n ";"
	fi
	echo -n $RANDOM
done

echo -n "\")."
