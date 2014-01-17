to=300
l=0
if [ $# -ge 1 ]; then
	to=$1
fi
if [ $# -ge 2 ]; then
	l=$2
fi
./aggregateresults.sh $to 15 3 $1
