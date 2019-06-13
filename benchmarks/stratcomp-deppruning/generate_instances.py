import os
import shutil
from random import randint

if os.path.exists('instances'):
    shutil.rmtree('instances')

os.mkdir('instances')

num_inst = 10

min_comp = 10
max_comp = 100
step_comp = 10

products_factor = 10
owners_factor = 0.2


for size in range(min_comp, max_comp + 1, step_comp):
	for num in range(0, num_inst):
		f = open('instances/inst_' + str(size).zfill(3) + '_' + str(num).zfill(3) + '.hex', 'w')

		f.write('strat(X1) v strat(X2) v strat(X3) v strat(X4) :- prod(_,X1,X2,X3,X4).\n')
		f.write('strat(X) :- &owns[\"instances/owners_' + str(size).zfill(3) + '_' + str(num).zfill(3) + '\",strat](X), comp(X).\n')

		for comp in range(0,size):
			f.write('comp(c' + str(comp) + ').\n')

		for prod in range(0,size * products_factor):
			f.write('prod(p' + str(prod) + ',c' + str(randint(0,size-1))  + ',c' + str(randint(0,size-1)) + ',c' + str(randint(0,size-1)) + ',c' + str(randint(0,size-1)) + ').\n')

		f.close()


		if True:

			f = open('instances/owners_' + str(size).zfill(3) + '_' + str(num).zfill(3), 'w')

			for owner in range(0,int(size * owners_factor)):
				first = randint(0,size-2)
				f.write('own(c' + str(first) + ',c' + str(randint(first+1,size-1)) + ',c' + str(randint(first+1,size-1)) + ',c' + str(randint(first+1,size-1)) + ').\n')

			f.close()

		else:
			f = open('instances/owners_' + str(size).zfill(3) + '_' + str(num).zfill(3), 'w')

			for owner in range(0,int(size * owners_factor)):
				f.write('own(c' + str(randint(0,size-1)) + ',c' + str(randint(0,size-1)) + ',c' + str(randint(0,size-1)) + ',c' + str(randint(0,size-1)) + ').\n')

			f.close()
