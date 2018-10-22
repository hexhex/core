for (( j = 1; j <= $1; j++ ))
do
	echo "y_node(n$j) v n_node(n$j).
node(X) :- y_node(X)."
done

echo "node(X) :- &adjacent[\"./instances/graphinst_nodecount_${2}_inst_${3}.graph\",node](X).
:- y_node(X), y_node(Y), y_node(Z), y_node(Z1), X != Y, Y != Z, Z != X, Z1 != Y, Z1 != Z, Z1 != X."
