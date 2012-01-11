% guess: build units from components (in worst case each unit is one component)
% +symmetry breaking
use(U,C) v nuse(U,C) :- component(U), component(C), U <= C.
% check: every component in one unit
:- not #count{ U : use(U,C) } = 1, component(C).
% deduce unit from use
unit(U) :- use(U,C).

% heuristics:
% an outer external component in a unit
% must not depend on other components in that unit
:- component(EC), outerext(EC), use(U,EC), use(U,C2), EC != C2, dep(EC,C2).

% define dependencies between units (necessary for more checks)
unitdep(U1,U2) :- use(U1,C1), use(U2,C2), U1 != U2, C1 != C2, dep(C1,C2).
% define transitive closure over unitdep
unitdep(U1,U3) :- unitdep(U1,U2), unitdep(U2,U3), U1 != U2, U2 != U3. % allow U1 = U3 on purpose

% check: no cyclic unit dependencies allowed
:- unitdep(U,U).

% as few units as possible
:~ unit(U). [1:3]
% use unit indices as small as possible
:~ unit(U), W=U+1. [W:2]
% units as large as possible (=> as many rules as possible in early units as possible)
rulecount_in_unit(U,Count) :- unit(U), Count = #count{ C : use(U,C) }, component(Count).
maxcount(Maxcount) :- Maxcount = #max {C : component(C) }.
:~ rulecount_in_unit(U,Count), ACount = Maxcount - Count, maxcount(Maxcount), W = U*ACount. [W:1]

% vim:syntax=prolog:
