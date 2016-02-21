var(exists,x1,1).
var(exists,x2,2).
var(all,y1,1).
var(all,y2,2).
k(3).

        {true(exists,X,C)} :- var(exists,X,C).
        :- not saturate.
        true(all,X,C) :- var(all,X,C), saturate.
        saturate :- f_sum(0,"!=",K), k(K).
        f_set(0,C,true(exists,X,C)) :- var(exists,X,C).
        f_set(0,C,true(all,X,C)) :- var(all,X,C).
    
