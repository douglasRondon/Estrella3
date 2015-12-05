

seq:
	@gcc sequencial.c -o seq
parallel:
	@mpicc paralelo.c -fopenmp -o parallel

run_seq:
	./seq
run_parallel:
	mpirun -map-by node -np 8 --hostfile hosts parallel

clean:
	@rm seq
	@rm parallel