#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <openssl/sha.h>
#include <math.h>
#include <string.h>

#define SIZE_HASH (sizeof(unsigned char) * 20)

char ABC[] = "abcdefghijklmnopqrstuvwxyz0123456789";

void printHash(unsigned char *md);
void get_chunk(int a, unsigned long long int b, int commsize, int rank, 
				unsigned long long int *lb, unsigned long long int *ub);
void getWord(unsigned long long int number, int size_word, int size_ABC, 
				unsigned char *word);
void brute_force_border(unsigned char *word, int size_word, double tstream);
void brute_force_step(unsigned char *word, int size_word, double tstream);

void printHash(unsigned char *md)
{
	for (int i = 0; i < 20; i++)
		printf("%x ", md[i]);
	printf("\n");
}

void get_chunk(int a, unsigned long long int b, int commsize, int rank, 
				unsigned long long int *lb, unsigned long long int *ub)
{
	int n = b - a + 1;
	int q = n / commsize;
	
	if (n % commsize) q++;
	
	int r = commsize * q - n;
	int chunk = q;
	
	if (rank >= commsize - r) { chunk = q - 1; }
	
	*lb = a;
	
	if (rank > 0) {
		if (rank <= commsize - r)
			*lb += q * rank;
		else
		*lb += q * (commsize - r) + (q - 1) * (rank - (commsize - r));
	}
	
	*ub = *lb + chunk - 1;
}

void getWord(unsigned long long int number, int size_word, int size_ABC, 
				unsigned char *word)
{
	int index_ABC;
	memset(word, 0, size_word);
	for (int i = 0; i < size_word; i++) {
		index_ABC = (number % (int) pow(size_ABC, i + 1)) / pow(size_ABC, i);
		word[size_word - i - 1] = ABC[index_ABC];
	}
}

void brute_force_border(unsigned char *word, int size_word, double tstream)
{
	int rank, commsize;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);

	unsigned char *hash = malloc(SIZE_HASH);

	int size_ABC = sizeof(ABC) - 1;
	
	SHA1(word, size_word, hash);

	unsigned char *brute_word = malloc(sizeof(unsigned char) * size_word);
	unsigned char *brute_hash = malloc(SIZE_HASH);

	unsigned long long int count_words = pow(size_ABC, size_word);

	unsigned long long int lb, ub;
	get_chunk(0, count_words - 1, commsize, rank, &lb, &ub);

	double t = MPI_Wtime();

	for (unsigned long long int i = lb; i < ub; i++) {
		getWord(i, size_word, size_ABC, brute_word);

		memset(brute_hash, 0, size_word);
		SHA1(brute_word, size_word, brute_hash);

		if (!memcmp(hash, brute_hash, SIZE_HASH)) {
			t = MPI_Wtime() - t;
			printf("Mission complete\n");
			printf("r = %d\n", rank);
			printf("t = %.6f\n", t);
			if (tstream != -1)
				printf("S = %.6f\n", tstream / t);
			printf("word = %s\n", brute_word);
			MPI_Abort(MPI_COMM_WORLD, 0);
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0)
		printf("Mission failed");

	free(brute_word);
	free(brute_hash);
}

void brute_force_step(unsigned char *word, int size_word, double tstream)
{
	int rank, commsize;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);

	unsigned char *hash = malloc(SIZE_HASH);

	int size_ABC = sizeof(ABC) - 1;
	
	SHA1(word, size_word, hash);

	unsigned char *brute_word = malloc(sizeof(unsigned char) * size_word);
	unsigned char *brute_hash = malloc(SIZE_HASH);

	unsigned long long int count_words = pow(size_ABC, size_word);

	double t = MPI_Wtime();

	for (unsigned long long int i = rank; i < count_words; i += commsize) {
		getWord(i, size_word, size_ABC, brute_word);

		memset(brute_hash, 0, size_word);
		SHA1(brute_word, size_word, brute_hash);

		if (!memcmp(hash, brute_hash, SIZE_HASH)) {
			t = MPI_Wtime() - t;
			printf("Mission complete\n");
			printf("r = %d\n", rank);
			printf("t = %.6f\n", t);
			if (tstream != -1)
				printf("S = %.6f\n", tstream / t);
			printf("word = %s\n", brute_word);
			MPI_Abort(MPI_COMM_WORLD, 0);
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0)
		printf("Mission failed");

	free(brute_word);
	free(brute_hash);
}

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	int rank, commsize;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);

	unsigned char *word;
	unsigned char *hash = malloc(SIZE_HASH);

	int size = 0;
	double tstream = -1;

	if (argc > 1) {
		while (argv[1][size] != '\0') size++;

		word = malloc(sizeof(unsigned char) * size);
		memcpy(word, argv[1], size);
	} else {
		if (rank == 0) {
			printf("Error\n");
			MPI_Abort(MPI_COMM_WORLD, 0);
		}

	}

	if (argc > 2)
		tstream = atof(argv[2]);

	// brute_force_border(word, size, tstream);
	brute_force_step(word, size, tstream);

	free(word);
	free(hash);

	MPI_Finalize();

	return 0;
}