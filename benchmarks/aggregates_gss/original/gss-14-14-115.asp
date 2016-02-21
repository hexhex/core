var(exists,x1,1).
var(exists,x2,2).
var(exists,x3,3).
var(exists,x4,4).
var(exists,x5,5).
var(exists,x6,6).
var(exists,x7,7).
var(exists,x8,8).
var(exists,x9,9).
var(exists,x10,10).
var(exists,x11,11).
var(exists,x12,12).
var(exists,x13,13).
var(exists,x14,14).
var(all,y1,1).
var(all,y2,2).
var(all,y3,3).
var(all,y4,4).
var(all,y5,5).
var(all,y6,6).
var(all,y7,7).
var(all,y8,8).
var(all,y9,9).
var(all,y10,10).
var(all,y11,11).
var(all,y12,12).
var(all,y13,13).
var(all,y14,14).
k(115).

        {true(exists,X,C)} :- var(exists,X,C).
        :- not saturate.
        true(all,X,C) :- var(all,X,C), saturate.
        saturate :- f_sum(0,"!=",K), k(K).
        f_set(0,C,true(exists,X,C)) :- var(exists,X,C).
        f_set(0,C,true(all,X,C)) :- var(all,X,C).
    
