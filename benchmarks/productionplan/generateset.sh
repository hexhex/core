# $1/$2: number of products min/max
# $3: number of requirements
# $4: maximum dependency sets per requirement
# $5: requirements probability
# $6: dependency probability
# $7: number of instances

if [[ $# -lt 7 ]]; then
	echo "Error: Script expects 7 parameters"
	exit 1;
fi

# create a directory for storing benchmark instances
mkdir -p instances
for (( cntProd=$1; cntProd <= $2; cntProd++ ))
do
	for (( inst=0; inst < $7; inst++ ))
	do
		cntReq=$3
		cntDepSetsPerReq=$4
		propReq=$5
		propDep=$6
		./generate.sh $cntProd $cntReq $cntDepSetsPerReq $propReq $propDep > "instances/inst_size_${cntProd}_inst_${inst}.hex"
	done
done

