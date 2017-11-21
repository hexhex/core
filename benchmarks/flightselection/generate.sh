# $1: possible savings
# $2: flights domain size
# $3: maximum dependencies per element
# $4: tag probability
# $5: dependency probability
# $6: maximum ressource constraints
# $7: constraint probability
# $8: constraint element probability
# $9: ressource domain size

# Example: ./generate.sh 10 2 10 50 10 100 50 5

# domain
for (( s=1; s <= $1; s++ ))
do
	echo -n "savings(s$s). "
done
echo ""

# requirements
propReq=$((32768 * $4 / 100))
propDep=$((32768 * $5 / 100))
propCons=$((32768 * $7 / 100))
propConsElem=$((32768 * $8 / 100))
for (( f=1; f<=$2; f++ ))
do
	# another requirement?
	for (( rc=1; rc <= $3; rc++ ))
	do
		if [[ $RANDOM -le $propReq ]]; then
			first=1
			for ((s=1; s<=$1; s++ ))
			do
				if [[ $RANDOM -le $propDep ]]; then
					if [[ $first == 1 ]]; then
						echo -n "savingsToFlights(f$f,p"
						first=0
					fi
					echo -n ",s$s"
				fi
			done
			if [[ $first == 0 ]]; then
				echo -n ",n"
			fi
			for ((s=1; s<=$1; s++ ))
			do
				if [[ $RANDOM -le $propDep ]]; then 
					if [[ $first == 1 ]]; then
						echo -n "savingsToFlights(f$f,p,n"
						first=0
					fi
					echo -n ",s$s"
				fi
			done
			if [[ $first == 0 ]]; then
				echo "). "
			fi
		fi
	done
done

for (( r=1; r<=$9; r++ ))
do
        # another requirement?
        for (( rc=1; rc <= $3; rc++ ))
        do
                if [[ $RANDOM -le $propReq ]]; then
                        first=1
                        for ((f=1; f<=$2; f++ ))
                        do
                                if [[ $RANDOM -le $propDep ]]; then
                                        if [[ $first == 1 ]]; then
                                                echo -n "flightsToRessources(r$r,p"
                                                first=0
                                        fi
                                        echo -n ",f$f"
                                fi
                        done
                        if [[ $first == 0 ]]; then
                                echo -n ",n"
                        fi
                        for ((f=1; f<=$2; f++ ))
                        do
                                if [[ $RANDOM -le $propDep ]]; then
                                        if [[ $first == 1 ]]; then
                                                echo -n "flightsToRessources(r$r,p,n"
                                                first=0
                                        fi
                                        echo -n ",f$f"
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
		for ((r=1; r<=$2; r++ ))
		do
#			if [[ $RANDOM -le 16384 ]]; then
#				naf=" not"
#			else
#				naf=""
#			fi
			if [[ $RANDOM -le $propConsElem ]]; then
				if [[ $first == 1 ]]; then
					echo -n ":-$naf required(r$r)"
					first=0
				else
					echo -n ",$naf required(r$r)"
				fi
			fi
		done
		if [[ $first == 0 ]]; then
			echo ". "
		fi
	fi
done

