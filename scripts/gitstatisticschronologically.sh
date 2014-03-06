if [ $# -ne 1 ]; then
	echo "Please specify output directory in \$1" >&2
	exit 1
fi
dir=$1

if [ ! -e $dir ]; then
	echo "Extracting information from git, this may take some time" >&2
	i=0
	tmpscript=$(mktemp)
	mkdir $dir
	cp scripts/gitstatistics.sh $dir/$tmpscript
	chmod a+x $dir/tmpscript

	git log | egrep "^commit|Date:" | sed 's/commit //' | while read line
	do
		read date
		git checkout $line
		echo $date > $dir/$i.txt
		$dir/$tmpscript >> $dir/$i.txt
		let i=i+1
	done
	rm $tmpdir/$tmpscript
fi

echo "Transforming statistics" >&2
for i in $dir/*.txt
do
	date=$(cat $i | egrep "Date" | sed 's/^Date: \(.*\)$/\1/' | sed 's/\+.*//')
	date=$(date -d "$date" +"%d.%m.%y %H:%M:%S")
	data=$(echo -e "${data}$date")
	txt=$(cat $i | egrep "%" | sed 's/[0-9]*\.[0-9]* %//' | sed "s/^\([0-9][0-9]*\)[ \t]*\(.*\)/,\2,\1/")
	data="${data}$(echo $txt | sed 's/ ,/,/g')\n"
done
contributors=$(echo $data | cut -d',' -f2,4,6,8,10,12,14,16,18,20,22 | sed 's/,/\n/g' | sort | uniq)

echo "Building output" >&2
echo "$data" | while read line
do
	date=$(echo $line | cut -d',' -f1)
	echo -n $date
	echo "$contributors" | while read contributor
	do
		con=$(echo $line | egrep -o "$contributor,[0-9]*")
		echo -n ",$con"
	done
	echo ""
done
