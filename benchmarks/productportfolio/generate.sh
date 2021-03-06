# $1: companies
# $2: products
# $3: maximum possibilities to produce a product
# $4: probability to include a company resp. produce in the dependency
# $5: dependency probability
# $6: maximum constraints
# $7: constraint probability
# $8: constraint element probability

# Example: ./generate.sh 10 2 10 50 10 100 50 5

# domain
for (( d=1; d <= $1; d++ ))
do
	echo -n "company(c$d). "
done
echo ""

# production plan
propReq=$((32768 * $4 / 100))
propDep=$((32768 * $5 / 100))
propCons=$((32768 * $7 / 100))
propConsElem=$((32768 * $8 / 100))
for (( t=1; t<=$2; t++ ))
do
	# another possibility to produce this product?
	for (( rc=1; rc <= $3; rc++ ))
	do
		if [[ $RANDOM -le $propReq ]]; then
			first=1
			for ((d=1; d<=$1; d++ ))
			do
				if [[ $RANDOM -le $propDep ]]; then
					if [[ $first == 1 ]]; then
						echo -n "productionplan(p$t,p"
						first=0
					fi
					echo -n ",c$d"
				fi
			done
                        for ((d=1; d<=$2; d++ ))
                        do
                                if [[ $RANDOM -le $propDep ]]; then
                                        if [[ $first == 1 ]]; then
                                                echo -n "productionplan(p$t,p"
                                                first=0
                                        fi
                                        echo -n ",p$d"
                                fi
                        done
			if [[ $first == 0 ]]; then
				echo -n ",n"
			fi
			for ((d=1; d<=$1; d++ ))
			do
				if [[ $RANDOM -le $propDep ]]; then 
					if [[ $first == 1 ]]; then
						echo -n "productionplan(p$t,p,n"
						first=0
					fi
					echo -n ",c$d"
				fi
			done
                        for ((d=1; d<=$2; d++ ))
                        do
                                if [[ $RANDOM -le $propDep ]]; then
                                        if [[ $first == 1 ]]; then
                                                echo -n "productionplan(p$t,p,n"
                                                first=0
                                        fi
                                        echo -n ",p$d"
                                fi
                        done
			if [[ $first == 0 ]]; then
				echo "). "
			fi
		fi
	done
done

# another constraint?
for (( rc=1; rc <= $6; rc++ ))
do
	if [[ $RANDOM -le $propCons ]]; then
		first=1
		for ((t=1; t<=$2; t++ ))
		do
			if [[ $RANDOM -le $propConsElem ]]; then
				if [[ $first == 1 ]]; then
					echo -n ":-$naf product(p$t)"
					first=0
				else
					echo -n ",$naf product(p$t)"
				fi
			fi
		done
		if [[ $first == 0 ]]; then
			echo ". "
		fi
	fi
done

