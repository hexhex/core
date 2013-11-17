% component 0:
%  innerRules:
%   in_ww(X) :- dom(X), not out_ww(X).
%   out_ww(X) :- dom(X), not in_ww(X).
component(0).
rules(0).
disjheads(0).
innerextnonmon(0).
outerextnonmon(0).
% component 1:
%  innerRules:
%   dl_pc_1(ww,X) :- in_ww(X).
component(1).
rules(1).
disjheads(1).
negcycles(1).
innerextnonmon(1).
outerextnonmon(1).
% component 2:
%  outerEatoms:
%   &testA[dl_pc_1](X)
component(2).
outerext(2).
disjheads(2).
negcycles(2).
innerextnonmon(2).
% component 3:
%  innerRules:
%   p_ww(X) :- &testA[dl_pc_2](X), dom(X), not &testA[dl_pc_1](X).
%   dl_pc_2(ww,X) :- p_ww(X).
%  innerEatoms:
%   &testA[dl_pc_2](X)
component(3).
rules(3).
innerext(3).
disjheads(3).
outerextnonmon(3).
% component 4:
%  outerEatoms:
%   &testA[dl_pc_2](X)
component(4).
outerext(4).
disjheads(4).
negcycles(4).
innerextnonmon(4).
% component 5:
%  outerEatoms:
%   &testA[dl_pc_1](X)
component(5).
outerext(5).
disjheads(5).
negcycles(5).
innerextnonmon(5).
% component 6:
%  outerEatoms:
%   &testA[dl_pc_2](X)
component(6).
outerext(6).
disjheads(6).
negcycles(6).
innerextnonmon(6).
% component 7:
%  innerRules:
%   fail :- in_ww(X), not fail, not &testA[dl_pc_2](X).
%   fail :- out_ww(X), &testA[dl_pc_1](X), not fail.
%   fail :- out_ww(X), &testA[dl_pc_2](X), not fail.
component(7).
rules(7).
disjheads(7).
innerextnonmon(7).
outerextnonmon(7).
% component 8:
%  innerConstraints:
%   :- p_ww(X), dl_pc_2(ww,X).
component(8).
constraints(8).
disjheads(8).
negcycles(8).
innerextnonmon(8).
outerextnonmon(8).
% dependency from 1 to 0.
dep(1,0).
posrule(1,0).
% dependency from 2 to 1.
dep(2,1).
extpred(2,1).
% dependency from 3 to 2.
dep(3,2).
posext(3,2).
negext(3,2).
% dependency from 4 to 3.
dep(4,3).
extpred(4,3).
% dependency from 5 to 1.
dep(5,1).
extpred(5,1).
% dependency from 6 to 3.
dep(6,3).
extpred(6,3).
% dependency from 7 to 4.
dep(7,4).
posext(7,4).
negext(7,4).
% dependency from 7 to 0.
dep(7,0).
posrule(7,0).
% dependency from 7 to 5.
dep(7,5).
posext(7,5).
negext(7,5).
% dependency from 7 to 6.
dep(7,6).
posext(7,6).
negext(7,6).
% dependency from 8 to 3.
dep(8,3).
posconstraint(8,3).
