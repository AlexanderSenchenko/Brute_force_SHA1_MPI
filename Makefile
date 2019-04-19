all:
	mpicc main.c -o main -L/whatever/path -lssl -lcrypto -lm