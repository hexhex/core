for (( size=$1; size <= $2; size+=$3 ))
do
			for (( inst=0; inst < ${4}; inst++ ))
			do
				ac=`printf "%03d" ${size}`
				in=`printf "%03d" ${inst}`
				./generate.sh $size > "instances/listinst_size_${ac}_inst_${in}.hex"
			done
done
