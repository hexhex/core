#!/bin/sh

echo "----------------------------------------"
echo "Number of lines in the current repository contributed by author:"
echo " (only lines currently in the repository, i.e., without removed lines)"
echo ""
lineperauthor=$(git ls-tree -r HEAD | cut -f 2 | grep -E '\.(cc|h|cpp|hpp|c|tcc)$' | xargs -n1 git blame --line-porcelain | egrep "^author ")
total=$(echo "$lineperauthor" | wc -l)
lineperauthor=$(echo "$lineperauthor" | sort | uniq -c | sort -nr)
echo "$lineperauthor" | while read line
do
	lines=$(echo $line | cut -d' ' -f1)
	name=$(echo $line | cut -d' ' -f3-)
	perc=$(echo "scale=2; 100*$lines/$total" | bc | sed 's/^\./0\./')
	perc=$(printf "%.2f" "$perc" | sed 's/^\(.\.\)/ \1/')
	echo "$lines \t $perc % \t $name"
done

echo "----------------------------------------"
echo "Total: $total lines of code"
echo "----------------------------------------"
