# $1: timeout

if [ $# -le 0 ]; then
	to=300
else
	to=$1
fi

cd instances
for instance in *.hex
do
	echo "
		Executable = ../benchmark_single.sh
		Universe = vanilla
		output = $instance.out
		error = $instance.error
		Log = $instance.log
		Requirements = machine == \"lion.kr.tuwien.ac.at\"
		request_memory = 8192 
		Initialdir = $PWD
		notification = never

		# queue
		request_cpus = 1 
		Arguments = $PATH $LD_LIBRARY_PATH $instance $to
		Queue 1
	     " > p.job
	condor_submit p.job
done

