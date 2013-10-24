to="300.00"
if [ $# -le 2 ]; then
	echo "Wrong number of arguments"
	exit 1;
fi

to=$1
extrstart=$2
extrlen=$3
totex=0
if [ $# -ge 4 ]; then
	totex=$4
fi

aggregate="
library(doBy);

t <- read.table('stdin',header=FALSE,as.is=TRUE)

# extract odd and even columns
odd <- c(1,seq(3,ncol(t),2))
even <- c(1,2,seq(4,ncol(t),2))

# compute means of odd and sums of even columns
means <- summaryBy(.~V1, data=t[,odd], FUN=mean)
sums <- summaryBy(.~V1, data=t[,even], FUN=sum)

#interleave the columns again
merged <- merge(means,sums)
g <-    function(x){
                 if ( x == 1 ){
                        return (1);
                }else if ( x == 2 ){
                        return (ncol(means) + 1);
                }else{
                        if (x %% 2 != 0 ){
                                return (1 + (x - 1) / 2);
                        }else{
                                return (ncol(means) + x / 2);
                        }
                }
        }

mixed <- sapply(seq(1,ncol(merged)), FUN=g)
merged <- merged[mixed]

# round all values in odd columns except in column 1
output <- merged
odd <- seq(3,ncol(output),2)
output[odd] <- round(output[odd],2)

write.table(format(output, nsmall=2, scientific=FALSE), , , FALSE, , , , , FALSE, FALSE)
"

while read line
do
	read -a array <<< "$line"
	if [[ $line != \#* ]]; then
		fn=${array[0]}
		if [ $extrlen -ge 1 ]; then
			array[0]="${fn:$extrstart:$extrlen} 1"
		else
			array[0]="${array[0]} 1"
		fi
		line=$(echo ${array[@]} | grep -v "#" | sed "s/\ \([0-9]*\)\.\([0-9]*\)/ \1.\2 0/g" | sed "s/--- --- ---/$to 1 $to 1 0 0/g")
		file=$(echo "$file\n$line")
	fi
done
if [ $totex -ge 1 ]; then
	# 1. encapsulate every second word in () and append &
	# 2. replace & at the end of the line with \\
	echo -e $file | Rscript <(echo "$aggregate") | sed "s/ *\(\S*\) *\(\S*\) */ \1 (\2) \& /g" | sed "s/\& *$/\\\\\\\\/g"
else
	echo -e $file | Rscript <(echo "$aggregate")
fi
