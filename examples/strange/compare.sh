#!/bin/bash

#~/Documents/dlvhex/dlvhex/branches/dlvhex-refactoring/build_clang/testsuite/TestMCSIEND easy offline foo.hex 2>foo.err.out |sort >foo.sort.out
#~/Documents/dlvhex/dlvhex/branches/dlvhex-refactoring/build_clang/testsuite/TestMCSIEND easy offline b2.hex 2>b2.err.out |sort >b2.sort.out
#diff foo.sort.out b2.sort.out

#~/Documents/dlvhex/dlvhex/branches/dlvhex-refactoring/build_gcc/testsuite/TestMCSIEND easy offline orig.hex 2>foo.off.err.out |sort >foo.off.sort.out
#~/Documents/dlvhex/dlvhex/branches/dlvhex-refactoring/build_gcc/testsuite/TestMCSIEND easy online orig.hex 2>foo.on.err.out |sort >foo.on.sort.out

~/Documents/dlvhex/dlvhex/branches/dlvhex-refactoring/build_gcc/testsuite/TestMCSIEND easy offline orig.hex 2>foo.off.err.out |sort >foo.off.sort.out
~/Documents/dlvhex/dlvhex/branches/dlvhex-refactoring/build_gcc/testsuite/TestMCSIEND easy online orig.hex 2>foo.on.err.out |sort >foo.on.sort.out
echo "problem?"
diff foo.on.sort.out foo.off.sort.out
echo "dbg comp off?"
diff foo.off.sort.out dbg.off.sort.out
echo "dbg comp on?"
diff foo.on.sort.out dbg.on.sort.out
