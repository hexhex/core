for file in src/*.cpp include/dlvhex2/*.h include/dlvhex2/*.tcc
do
	cat $file | bcpp -bcl -s -i 4 > tmp
	mv tmp $file
done
