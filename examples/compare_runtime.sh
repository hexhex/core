echo "Translation:"
echo "Runtime:"
/usr/bin/time -f %E ../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ $1 >/dev/null
echo ""

echo "Genuine without ext. learning:"
echo "Runtime:"
/usr/bin/time -f %E ../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver $1 >/dev/null
../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver --verbose=1 $1 >/dev/null 2>out.gen.txt
cat out.gen.txt | grep -A 7 "Final Statistics" > statistics.gen.txt
cat statistics.gen.txt
echo ""

echo "Genuine with ext. learning (no special learning rules):"
echo "Runtime:"
/usr/bin/time -f %E ../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver --extlearn $1 >/dev/null
../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver --extlearn=none --verbose=1 $1 >/dev/null 2>out.genlearn.txt
cat out.genlearn.txt | grep -A 7 "Final Statistics" > statistics.genlearn.txt
cat statistics.genlearn.txt
echo ""

echo "Genuine with ext. learning (partial):"
echo "Runtime:"
/usr/bin/time -f %E ../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver --extlearn $1 >/dev/null
../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver --extlearn=partial --verbose=1 $1 >/dev/null 2>out.genlearn.txt
cat out.genlearn.txt | grep -A 7 "Final Statistics" > statistics.genlearn.txt
cat statistics.genlearn.txt
echo ""

echo "Genuine with ext. learning (functionality):"
echo "Runtime:"
/usr/bin/time -f %E ../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver --extlearn $1 >/dev/null
../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver --extlearn=functionality --verbose=1 $1 >/dev/null 2>out.genlearn.txt
cat out.genlearn.txt | grep -A 7 "Final Statistics" > statistics.genlearn.txt
cat statistics.genlearn.txt
echo ""

echo "Genuine with ext. learning (all special learning rules):"
echo "Runtime:"
/usr/bin/time -f %E ../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver --extlearn $1 >/dev/null
../src/dlvhex/dlvhex --plugindir=../testsuite/.libs/ --internalsolver --extlearn --verbose=1 $1 >/dev/null 2>out.genlearn.txt
cat out.genlearn.txt | grep -A 7 "Final Statistics" > statistics.genlearn.txt
cat statistics.genlearn.txt
echo ""

