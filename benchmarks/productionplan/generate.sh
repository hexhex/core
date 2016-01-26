# $1: number of products
# $2: number of requirements
# $3: maximum dependency sets per requirement
# $4: requirements probability
# $5: dependency probability

# products
for (( p=1; p <= $1; p++ ))
do
	echo -n "product(prod$p). "
done

# requirements
propReq=$((32768 * $4 / 100))
propDep=$((32768 * $5 / 100))
for (( r=1; r <= $2; r++ ))
do
	# another requirement?
	for (( rc=1; rc <= $3; rc++ ))
	do
		if [[ $RANDOM -le $propReq ]]; then
			echo -n "require(req$r,p"
			for (( p=1; p<=$1; p++ ))
			do
				if [[ $RANDOM -le $propDep ]]; then 
					echo -n ",prod$p"
				fi
			done
			echo -n "). "
		fi
	done
done

