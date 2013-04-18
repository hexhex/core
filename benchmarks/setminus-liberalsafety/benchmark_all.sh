# $1: timeout
if [ $# -le 0 ]; then
	confstr="--extlearn --flpcheck=aufs prog$instance.hex;--extlearn --flpcheck=aufs --liberalsafety prognd$instance.hex"
else
	confstr=$1
fi
if [ $# -le 1 ]; then
	to=300
else
	to=$2
fi

for (( instance=1; instance<=20; instance++ ))
do
	echo "
		Executable = ./benchmark_single.sh
		Universe = vanilla
		output = $instance.out
		error = $instance.error
		Log = $instance.log
		Requirements = machine == \"node5.kr.tuwien.ac.at\"
		request_memory = 4096 
		Initialdir = $PWD
		notification = never

		# queue
		request_cpus = 1 
		Arguments = $PATH $LD_LIBRARY_PATH $instance $confstr $to
		Queue 1
	     " > p.job
	condor_submit p.job
done

