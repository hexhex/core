% Guess colours.
chosenColour(N,C) v notChosenColour(N,C) :- node(N), colour(C).

% At least one color per node.
:- node(X), not colored(X).
colored(X) :- chosenColour(X,Fv1).

% Only one color per node.
:- chosenColour(N,C1), chosenColour(N,C2), C1!=C2. 

% No two adjacent nodes have the same colour. 
:- link(X,Y),  X<Y, chosenColour(X,C), chosenColour(Y,C).

