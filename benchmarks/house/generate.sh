# $1: node count
# $2: edge propability
m=$(($1 * 3))
#echo "#maxint=$m."

let cabCnt="$1 + 2"
let roomCnt="$1 + 1"
let thingCnt="$1 * 2"
let unassignedThingCnt="2"

for (( i=1; i <= $1; i++ ))
do
	echo "person($i)."
done

for (( i=1; i <= $cabCnt; i++ ))
do
        echo "cabinet($i)."
done

for (( i=1; i <= $roomCnt; i++ ))
do
        echo "room($i)."
done

nextFreeCab=1
nextFreeRoom=1

for (( i=1; i <= $thingCnt; i++ ))
do
	echo "thing($i)."
	p="$((1 + $1 * $RANDOM / 32768))"
	echo "personTOthing($p,$i)."

	if [[ $i -gt $unassignedThingCnt ]]; then

	        # is cabinet full?
        	if [[ ${thingsInCabOfPerson[$p]} -ge 6 ]]; then
			thingsInCabOfPerson[$p]=0
        	fi

		# has this person a cabinet?
		if [[ ${thingsInCabOfPerson[$p]} -eq 0 ]];
		then
			if [[ $nextFreeCab -le $cabCnt ]]; then
				cabOfPerson[$p]=$nextFreeCab
				let nextFreeCab=nextFreeCab+1
				echo "cabinetTOperson(${cabOfPerson[$p]}, $p)."

				# is room full?
				if [[ ${cabsInRoomOfPerson[$p]} -ge 5 ]]; then
					cabsInRoomOfPerson[$p]=0
				fi

				# has this person a room?
				if [[ ${cabsInRoomOfPerson[$p]} -eq 0 ]];
				then
					if [[ $nextFreeRoom -le $roomCnt ]]; then
						roomOfperson[$p]=$nextFreeRoom
						let nextFreeRoom=nextFreeRoom+1
						echo "roomTOcabinet(${roomOfperson[$p]}, ${cabOfPerson[$p]})."
					fi
				fi
			fi
		fi

		if [[ cabOfPerson[$p] -ge 0 ]]; then
			echo "cabinetTOthing(${cabOfPerson[$p]}, $i)."
			let thingsInCabOfPerson[$p]="${thingsInCabOfPerson[$p]}+1"
		fi
	fi
done
