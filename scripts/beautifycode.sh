for file in src/*.cpp include/dlvhex2/*.h include/dlvhex2/*.tcc
do
	cat $file | bcpp -bnl -s -i 2 > tmp
	mv tmp $file
done
