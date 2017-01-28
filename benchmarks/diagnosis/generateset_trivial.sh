# $1: instance size upper limit

if [[ $# -lt 1 ]]; then
        echo "Error: Script expects 1 parameter"
        exit 1;
fi

for (( size=1; size <= $1; size++ ))
do

	echo "
		#maxint=$size.
		product(X) :- #int(X).
		require(a,p,0).
		require(a,p,X) :- #int(X), X > 2.
		require(b,p,1).
		require(b,p,X) :- #int(X), X >= 2.
		" > instances/inst_size_${size}_inst_0.hex
done

