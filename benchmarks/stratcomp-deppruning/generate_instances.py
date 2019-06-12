import os
import shutil
from random import randint

if os.path.exists('instances'):
    shutil.rmtree('instances')

os.mkdir('instances')

num_inst = 10

min_comp = 1
max_comp = 100
step_comp = 1

products_factor = 5
owners_factor = 0.2


for size in range(min_comp, max_comp + 1, step_comp):
	for num in range(0, num_inst):
		f = open('instances/inst_' + str(size) + '_' + str(num) + '.hex', 'w')

		f.write('strat(X1) v strat(X2) :- prod(_,X1,X2).\n')
		f.write('strat(X) :- &owns[\"instances/owners_' + str(size) + '_' + str(num) + '\",strat](X), comp(X).\n')

		for comp in range(0,size):
			f.write('comp(c' + str(comp) + ').\n')

		for prod in range(0,size * products_factor):
			f.write('prod(p' + str(prod) + ',c' + str(randint(0,size-1))  + ',c' + str(randint(0,size-1)) + ').\n')

		f.close()


		f = open('instances/owners_' + str(size) + '_' + str(num), 'w')

		for owner in range(0,int(size * owners_factor)):
			f.write('owns(c' + str(randint(0,size-1))  + ',c' + str(randint(0,size-1)) + ').\n')

		f.close()

