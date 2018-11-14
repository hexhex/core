import random

for goods_num in range(1,11):
	for inst_num in range(1,11):

		prefs1 = list(range(0,goods_num))
		prefs1 = random.sample(prefs1,len(prefs1))

		prefs2 = list(range(0,goods_num))
		prefs2 = random.sample(prefs1,len(prefs1))

		f1 = open('instances/goodnum_' + str(goods_num).zfill(3) + '_instnum_' + str(inst_num).zfill(3) + '.pref', 'w')

		f1.write(str(goods_num))
		f1.write('\n')

		for p1 in prefs1:
			f1.write(str(p1))
			f1.write('\n')

		for p2 in prefs2:
			f1.write(str(p2))
			f1.write('\n')

		f1.close()

		f2 = open('instances/goodnum_' + str(goods_num).zfill(3) + '_instnum_' + str(inst_num).zfill(3) + '.hex', 'w')



		for i in range(0,goods_num):
			f2.write('position(p' + str(i) + ').')
			f2.write('\n')

		for i in range(0,goods_num):
			f2.write('item(i' + str(i) + ').')
			f2.write('\n')

		f2.write('pref_file("instances/goodnum_' + str(goods_num).zfill(3) + '_instnum_' + str(inst_num).zfill(3) + '.pref").')

		f2.close()


