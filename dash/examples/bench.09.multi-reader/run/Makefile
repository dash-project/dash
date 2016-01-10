
all : 

supermuc : supermuc-hw

supermuc-hw :
	for nodes in 25 50 75 100 125 150 175 200 225 250 275 300 325 350 375 400 425 450 475 500; do    \
	./template.pl DIR=`pwd`,NNODES=$$nodes,CLASS=general supermuc-hw.ll > run-$$nodes.job; \
	done

#	./template.pl DIR=`pwd`,NNODES=5,CLASS=micro      supermuc-hw.ll > run-5.job
#	./template.pl DIR=`pwd`,NNODES=10,CLASS=micro     supermuc-hw.ll > run-10.job
#	./template.pl DIR=`pwd`,NNODES=15,CLASS=micro     supermuc-hw.ll > run-15.job
#	./template.pl DIR=`pwd`,NNODES=20,CLASS=micro     supermuc-hw.ll > run-20.job

cori : 
	for nodes in 2 4 8 16 32 64 128 256; do    \
	./template.pl DIR=`pwd`,NNODES=$$nodes cori.slurm > cori-run-$$nodes.job; \
	done


clean :
	rm -f *~
	rm -f *.job

submit :
	find . -name "supermuc-run-*.job" -exec llsubmit {} \;
	find . -name "cori-run-*.job" -exec sbatch {} \;
