if [ $# -le 3 ]; then
	echo "Wrong number of arguments"
	exit 1;
fi

filecnt=$1
filestart=2
fileend=$((filestart+filecnt-1))

i=$((fileend+1))
colcnt=${!i}
colstart=$((fileend+2))
colend=$((colstart+colcnt-1))

i=$((colend+1))
tolatex=${!i}

# initialization
edit="
library(doBy);
tables <- list();
"

# read all tables
fn=0
for (( i=$filestart; i <= $fileend; i++ )); do
	fn=$((fn+1))
	edit="$edit
	tables[[$fn]] <- read.table('${!filestart}',header=FALSE,as.is=TRUE);
	"
done

# merge them
edit="$edit
merged <- data.frame(tables);
sel <- rep(NA, $colcnt * 2);
"
col=0
for (( i=$colstart; i<=$colend; i++ )); do
	edit="$edit
	valcol <- 1 + $col * 2;
	tocol <- valcol + 1;
	sel[valcol:valcol] <- 1 + (${!i} - 1) * 2;
	sel[tocol:tocol] <- sel[valcol:valcol] + 1;
	"
	col=$((col+1))
done

# output processing
edit="$edit
output <- merged[,sel];
write.table(format(output, nsmall=2, scientific=FALSE), , , FALSE, , , , , FALSE, FALSE)
"

# 1. encapsulate every second number with ()
# 2. replace & at the end of the line with \\
if [ $tolatex -ge 1 ]; then
	echo -e $file | Rscript <(echo "$edit") | sed "s/ *\(\S*\) *\(\S*\) */ \1 (\2) \& /g" | sed "s/\& *$/\\\\\\\\/g"
else
	echo -e $file | Rscript <(echo "$edit")
fi
