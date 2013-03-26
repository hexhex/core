node(X) :- edge(X, Y).
node(Y) :- edge(X, Y).
reachable(X, X) :- node(X).
reachable(X, Y) :- reachable(X, Z), edge(Z, Y).
unreachable(X, Y) :- node(X), node(Y), not reachable(X, Y).
