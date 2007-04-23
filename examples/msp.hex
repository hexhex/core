root(a).
node(a). node(b). node(c). node(d). node(e).
edge(a,b,4). edge(a,c,3). edge(c,b,2). edge(c,d,3). edge(b,e,4). edge(d,e,5).

in_tree(X,Y,C) v out_tree(X,Y) :- edge(X,Y,C), reached(X).
:- root(X), in_tree(_,X,C).
:- in_tree(X,Y,C), in_tree(Z,Y,C), X != Z.

reached(X):- root(X).
reached(Y):- reached(X), in_tree(X,Y,C).
:- node(X), not reached(X).

:~ in_tree(X,Y,C). [C:1]
