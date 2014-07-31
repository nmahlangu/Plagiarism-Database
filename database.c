/*************************************************************************************************
 *  database.c
 *  Nicholas Mahlangu
 *  A database that compares document similarities.
 * 
 *  Basic Features
 *  - Uses permutations on 64-bit numbers to compare document similarity
 *  - Any document can be added by inserting it into the db/ folder and
 *    adding its name to the textfile "init.txt"
 *  
 *  Additional Features
 *	- Option 1: Can run the database normallly
 *  - Option 2: Can run 2 files multiple times for comparison and average the results for accuracy
 *	- Option 3: Can run a file against every other file in the database and see which one it's most
 *              similar to with a GUI (bars)
 *  - Additional Feature: Can vary the number of permutations used when comparing two files
 **************************************************************************************************/
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "MurmurHash2.h"

// constants
#define SHINGLE_LENGTH 2            // length of a shingle
#define PERMUTATIONS 4000           // biggest number for a permutation result
#define RUNS 5						// number of times to run and average a comparison of two files
int seed;                           // seed for MurmurHash2

// function prototype
char** boot(void);														   // starts the database (returns all the files)
void option_1(void);                                                       // averages the results of shingling RUNS times
void option_2(void);													   // runs the database normally
void option_3(void);												       // compares a file with every other file
char* get_next_word(FILE* file);                                           // gets the next word from a given file pointer
char* concatenate_circle(int head, char** circle, int* circle_lens);       // concatenates all of the words in the circle
uint64_t MurmurHash64A (const void* key, int len, uint64_t seed);		   // a complicated 64-bit hash function
float permute_and_compare(char* file_1, int set_1_len, uint64_t* set_1,    // permutes 2 sets of 64-bit numbers
         			     char* file_2, int set_2_len, uint64_t* set_2);

// main
int main(void)
{
	// get files in the database
	char** files = boot();
	int num_files = 0;
	FILE* fp = fopen("init.txt", "r");
	char ch;
	while(1)
	{
		ch = fgetc(fp);
		if (feof(fp))
			break;
		else if (ch == '\n')
			num_files++;
	}
	fclose(fp);
	
	// print crpytic symbols for aesthetics
	printf("\033[2J");
    printf("\033[%d;%dH", 0, 0);

    // seed for MurmurHash2
    srand(time(NULL));

    // print UI   
    int spaces = 42;
    printf("******************************** Plagiarism Database ********************************\n");
    printf("*                     Files available to check are listed below.                    *\n");
    for (int i = 0; i < num_files; i++)
    {
    	int word_len = strlen(files[i]);
    	printf("*");
    	for (int j = 0; j < spaces - (word_len / 2); j++)
    		printf(" ");
    	printf("%s", files[i]);
    	for (int k = spaces - (word_len / 2) + word_len; k < 83; k++)
    		printf(" ");
    	printf("*\n");
    }
    printf("*************************************************************************************\n");

    // generate MurmurHash2 seed
    seed = rand();

    // for what the user wants to do 
    char* input;

    // clean up (don't need these anymore)
    for (int i = 0; i < num_files; i++)
    	free(files[i]);
    free(files);

    // execute database queries
    while(1)
    {
    	// print message
    	printf("What would you like to do?\n"
    		   "* 1 - Use the database\n"
    		   "* 2 - Compare two files `%d` times and average the results\n"
    		   "* 3 - Compare a file against every other file in the database\n"
    	       "# 4 - Quit\n", RUNS);
    	input = readline("Option: ");
    	int input_num = atoi(&input[0]);

    	// call get_files appropriately
    	if (input_num == 2)
    		option_1();
    	else if (input_num == 1)
    		option_2();
    	else if (input_num == 3)
    		option_3();
    	else if (input_num == 4)
    	{
    		printf("Quitting... goodbye\n");
    		exit(1);
    	}
    	else
    		printf("Please pick one of the stated options\n\n");

    	// for CPU
    	usleep(20000); 		
    	free(input);	
    }
}

/*
 *  boot
 *  Starts the database (returns all the files)
 */
char** boot(void)
{
	// get files in the database
	FILE* init = fopen("init.txt", "r");
	if (init == NULL)
	{
		printf("Error, make sure `init.txt` is in the same directory as `database.c` and `text files`\n");
		fclose(init);
		exit(1);
	}
	char* char_buf = malloc(BUFSIZ * sizeof(char));
	int char_buf_index = 0;
	char ch;
	while(1)
	{
		ch = fgetc(init);
		if (feof(init))
			break;
		char_buf[char_buf_index++] = ch;
	}
	int num_files = 0;
	for (int i = 0, len = strlen(char_buf); i < len; i++)
	{
		if (char_buf[i] == '\n')
			num_files++;
	}
	char** files = malloc(num_files * sizeof(char*));
	char* file_buf;
	char* location;
	for (int i = 0; i < num_files; i++)
	{
		file_buf = (i == 0) ? strtok_r(char_buf, "\n", &location) : strtok_r(NULL, "\n", &location);
		files[i] = malloc(strlen(file_buf));
		strcpy(files[i], file_buf);
	}

	// return the result
	free(char_buf);
	fclose(init);
	return files;
}

/*
 *  option_1
 *  Averages the results of shingling RUNS times
 */
void option_1(void)
{
	// open the two files (lots of error checking)
	printf("\nEnter two files to compare.\n");
	char* file_a = readline("File 1: ");
    if (file_a == NULL)
    {
    	printf("Reached EOF.\n");
    	exit(1);
    }
    else if (strcmp(file_a, "Quit") == 0)
    {
    	printf("Quitting\n");
    	exit(2);
    }
	if (!isatty(fileno(stdin)))
        printf("File 1: %s\n", file_a);
	char* file_1 = malloc(strlen(file_a) + 3 * sizeof(char));
	file_1[0] = 'd'; file_1[1] = 'b'; file_1[2] = '/';
	strcpy(file_1 + 3, file_a);
	char* file_b = readline("File 2: ");
	if (file_b == NULL)
	{
		printf("Reached EOF.\n");
		exit(3);
	}
	if (!isatty(fileno(stdin)))
		printf("File 2: %s\n", file_b);
	if (strcmp(file_a, file_b) == 0)
	{
		printf("Please enter two different files.\n\n");
		return;
	}
	char* file_2 = malloc(strlen(file_b) + 3 * sizeof(char));
	file_2[0] = 'd'; file_2[1] = 'b'; file_2[2] = '/';
	strcpy(file_2 + 3, file_b);
	FILE* file_1_fp = fopen(file_1, "r");
	FILE* file_2_fp = fopen(file_2, "r");
	if (file_1_fp == NULL)
	{
		printf("Couldn't open `%s`, please enter a file that's listed in the database.\n\n", file_1);
		free(file_a);
		free(file_b);
		free(file_1);
		free(file_2);
		fclose(file_1_fp);
		fclose(file_2_fp);
		return;
	}
	else if (file_2_fp == NULL)
	{
		printf("Couldn't open `%s`, please enter a file that's listed in the database.\n\n", file_2);
		free(file_a);
		free(file_b);
		free(file_1);
		free(file_2);
		fclose(file_1_fp);
		fclose(file_2_fp);
		return;
	}

	// for keeping track result of each run
	float* results = malloc(RUNS * sizeof(float));
	int run = 1;

	// do process RUNS times
	for (int a = 0; a < RUNS; a++)
	{
		// re-seed
		seed = rand();
		printf("\rChecking (%d/%d)", run++, RUNS);
		fflush(stdout);

		// a circular array to store words
		char** circle = malloc(SHINGLE_LENGTH * sizeof(char*));
		int* circle_lens = malloc(SHINGLE_LENGTH * sizeof(int));
		int circle_index = 0;

		// variables for file 1
		int sz_unit = BUFSIZ * sizeof(uint64_t);
		uint64_t* f1_shingles = malloc(sz_unit);
		ssize_t f1_shingles_sz = sz_unit;
		int f1_shingles_count = 0;
		int f1_stored_shingles = 0;

		// get words from file 1
		char* word = get_next_word(file_1_fp);
		while (word != NULL)
		{
			// add word to the circle
			circle_lens[circle_index]= strlen(word);
			circle[circle_index] = malloc(circle_lens[circle_index]);
			strncpy(*(circle + circle_index), word, circle_lens[circle_index]);
			circle_index = (circle_index + 1) % SHINGLE_LENGTH;
			f1_shingles_count++;

			// shingle if ready
			if (f1_shingles_count >= SHINGLE_LENGTH)
			{
				// get a string and convert it to a 64-bit number
				char* str = concatenate_circle(circle_index, circle, circle_lens);
				uint64_t hash =  MurmurHash64A(str, strlen(str), seed);

				// possibly increase array size and then store number
				if (f1_shingles_count % BUFSIZ == 0 && f1_shingles_count != 0)
				{
					f1_shingles_sz += sz_unit;
					f1_shingles = realloc(f1_shingles, f1_shingles_sz);
				}
				f1_shingles[f1_stored_shingles++] = hash;
			}

			// get next word
			word = get_next_word(file_1_fp);
		}

		// reset circle
		for (int i = 0; i < SHINGLE_LENGTH; i++)
		{
			free(circle[i]);
			circle_lens[i] = 0;
		}
		circle_index = 0;

		// variables for file 2
		uint64_t* f2_shingles = malloc(sz_unit);
		ssize_t f2_shingles_sz = sz_unit;
		int f2_shingles_count = 0;
		int f2_stored_shingles = 0;

		// get words from file 2
		word = get_next_word(file_2_fp);
		while (word != NULL)
		{
			// add word to circle
			circle_lens[circle_index]= strlen(word);
			circle[circle_index] = malloc(circle_lens[circle_index]);
			strncpy(*(circle + circle_index), word, circle_lens[circle_index]);
			circle_index = (circle_index + 1) % SHINGLE_LENGTH;
			f2_shingles_count++;

			// shingle if ready
			if (f2_shingles_count >= SHINGLE_LENGTH)
			{
				// get a string and convert it to a 64-bit number
				char* str = concatenate_circle(circle_index, circle, circle_lens);
				uint64_t hash =  MurmurHash64A(str, strlen(str), seed);

				// possibly increase array size and then store number
				if (f2_shingles_count % BUFSIZ == 0 && f2_shingles_count != 0)
				{
					f2_shingles_sz += sz_unit;
					f2_shingles = realloc(f2_shingles, f2_shingles_sz);
				}
				f2_shingles[f2_stored_shingles++] = hash;
			}

			// get next words
			word = get_next_word(file_2_fp);
		}

		// reset circle
		for (int i = 0; i < SHINGLE_LENGTH; i++)
		{
			free(circle[i]);
			circle_lens[i] = 0;
		}
		circle_index = 0;

		// permute and compare file similarities
		results[a] = permute_and_compare(file_a, f1_shingles_count, f1_shingles, 
			                             file_b, f2_shingles_count, f2_shingles);

		// clean up
		free(f1_shingles);
	    free(f2_shingles);
	    free(circle);
	    free(circle_lens);

	    // seek back to the beginning of the files for next run
	    fseek(file_1_fp, 0, SEEK_SET);
	    fseek(file_2_fp, 0, SEEK_SET);
	}
	printf("\n");

	// calculate average and print message
	float acc = 0;
	for (int i = 0; i < RUNS; i++)
		acc += results[i];
	float final_result = acc / (float)RUNS;
	for (int i = 0; i < RUNS; i++)
		printf("* Run %d: %.2f\n", i + 1, results[i]);
	printf("Average of all %d rounds: %.2f\n\n", RUNS, final_result);

	// clean up
	fclose(file_1_fp);
	fclose(file_2_fp);
	free(file_a);
	free(file_b);
	free(file_1);
	free(file_2);
}

/*
 *  option_2
 *  Runs the database normally
 */
void option_2(void)
{
	// open the two files (lots of error checking)
	printf("\nEnter two files to compare.\n");
	char* file_a = readline("File 1: ");
    if (file_a == NULL)
    	exit(1);
    else if (strcmp(file_a, "Quit") == 0)
    {
    	printf("Quitting\n");
    	exit(2);
    }
	if (!isatty(fileno(stdin)))
        printf("File 1: %s\n", file_a);
	char* file_1 = malloc(strlen(file_a) + 3 * sizeof(char));
	file_1[0] = 'd'; file_1[1] = 'b'; file_1[2] = '/';
	strcpy(file_1 + 3, file_a);
	char* file_b = readline("File 2: ");
	if (file_b == NULL)
	{
		printf("Reached EOF.\n");
		exit(3);
	}
	if (!isatty(fileno(stdin)))
		printf("File 2: %s\n", file_b);
	if (strcmp(file_a, file_b) == 0)
	{
		printf("Please enter two different files.\n\n");
		return;
	}
	char* file_2 = malloc(strlen(file_b) + 3 * sizeof(char));
	file_2[0] = 'd'; file_2[1] = 'b'; file_2[2] = '/';
	strcpy(file_2 + 3, file_b);
	FILE* file_1_fp = fopen(file_1, "r");
	FILE* file_2_fp = fopen(file_2, "r");
	if (file_1_fp == NULL)
	{
		printf("Couldn't open `%s`, please enter a file that's listed in the database.\n\n", file_1);
		free(file_a);
		free(file_b);
		free(file_1);
		free(file_2);
		fclose(file_1_fp);
		fclose(file_2_fp);
		return;
	}
	else if (file_2_fp == NULL)
	{
		printf("Couldn't open `%s`, please enter a file that's listed in the database.\n\n", file_2);
		free(file_a);
		free(file_b);
		free(file_1);
		free(file_2);
		fclose(file_1_fp);
		fclose(file_2_fp);
		return;
	}

	// a circular array to store words
	char** circle = malloc(SHINGLE_LENGTH * sizeof(char*));
	int* circle_lens = malloc(SHINGLE_LENGTH * sizeof(int));
	int circle_index = 0;

	// variables for file 1
	int sz_unit = BUFSIZ * sizeof(uint64_t);
	uint64_t* f1_shingles = malloc(sz_unit);
	ssize_t f1_shingles_sz = sz_unit;
	int f1_shingles_count = 0;
	int f1_stored_shingles = 0;

	// get words from file 1
	char* word = get_next_word(file_1_fp);
	while (word != NULL)
	{
		// add word to the circle
		circle_lens[circle_index]= strlen(word);
		circle[circle_index] = malloc(circle_lens[circle_index]);
		strncpy(*(circle + circle_index), word, circle_lens[circle_index]);
		circle_index = (circle_index + 1) % SHINGLE_LENGTH;
		f1_shingles_count++;

		// shingle if ready
		if (f1_shingles_count >= SHINGLE_LENGTH)
		{
			// get a string and convert it to a 64-bit number
			char* str = concatenate_circle(circle_index, circle, circle_lens);
			uint64_t hash =  MurmurHash64A(str, strlen(str), seed);

			// possibly increase array size and then store number
			if (f1_shingles_count % BUFSIZ == 0 && f1_shingles_count != 0)
			{
				f1_shingles_sz += sz_unit;
				f1_shingles = realloc(f1_shingles, f1_shingles_sz);
			}
			f1_shingles[f1_stored_shingles++] = hash;
		}

		// get next word
		word = get_next_word(file_1_fp);
	}

	// reset circle
	for (int i = 0; i < SHINGLE_LENGTH; i++)
	{
		free(circle[i]);
		circle_lens[i] = 0;
	}
	circle_index = 0;

	// variables for file 2
	uint64_t* f2_shingles = malloc(sz_unit);
	ssize_t f2_shingles_sz = sz_unit;
	int f2_shingles_count = 0;
	int f2_stored_shingles = 0;

	// get words from file 2
	word = get_next_word(file_2_fp);
	while (word != NULL)
	{
		// add word to circle
		circle_lens[circle_index]= strlen(word);
		circle[circle_index] = malloc(circle_lens[circle_index]);
		strncpy(*(circle + circle_index), word, circle_lens[circle_index]);
		circle_index = (circle_index + 1) % SHINGLE_LENGTH;
		f2_shingles_count++;

		// shingle if ready
		if (f2_shingles_count >= SHINGLE_LENGTH)
		{
			// get a string and convert it to a 64-bit number
			char* str = concatenate_circle(circle_index, circle, circle_lens);
			uint64_t hash =  MurmurHash64A(str, strlen(str), seed);

			// possibly increase array size and then store number
			if (f2_shingles_count % BUFSIZ == 0 && f2_shingles_count != 0)
			{
				f2_shingles_sz += sz_unit;
				f2_shingles = realloc(f2_shingles, f2_shingles_sz);
			}
			f2_shingles[f2_stored_shingles++] = hash;
		}

		// get next words
		word = get_next_word(file_2_fp);
	}

	// reset circle
	for (int i = 0; i < SHINGLE_LENGTH; i++)
	{
		free(circle[i]);
		circle_lens[i] = 0;
	}
	circle_index = 0;

	// permute and compare file similarities
	float resemblance = permute_and_compare(file_a, f1_shingles_count, f1_shingles, 
		                             file_b, f2_shingles_count, f2_shingles);

	// print similarity report
	printf("Result:\n");
	printf("                            matching minimums         %.1f             \n", (PERMUTATIONS * resemblance));
	printf("           Similarity =   ---------------------  =  -------  = %.2f  \n", resemblance);
	printf("                          # calculated minimums       %.1f             \n\n", (float)PERMUTATIONS);
	
	// clean up
	free(circle);
	free(circle_lens);
	free(file_a);
	free(file_b);
	free(file_1);
	free(file_2);
	free(f1_shingles);
	free(f2_shingles);
	fclose(file_1_fp);
	fclose(file_2_fp);
}

/*
 *  option_3
 *  Checks a file against the rest of the files in the database
 */
void option_3(void)
{
	// get the files in the database
	char** files = boot();
	int num_files = 0;
	FILE* fp = fopen("init.txt", "r");
	char ch;
	while(1)
	{
		ch = fgetc(fp);
		if (feof(fp))
			break;
		else if (ch == '\n')
			num_files++;
	}
	fclose(fp);

	// open the file (lots of error checking)
	printf("\nEnter a file to check agains the rest of the database.\n");
	char* file_a = readline("File: ");
    if (file_a == NULL)
    	exit(1);
    else if (strcmp(file_a, "Quit") == 0)
    {
    	printf("Quitting...\n");
    	exit(2);
    }
	if (!isatty(fileno(stdin)))
        printf("File 1: %s\n", file_a);
	char* file_1 = malloc(strlen(file_a) + 3 * sizeof(char));
	file_1[0] = 'd'; file_1[1] = 'b'; file_1[2] = '/';
	strcpy(file_1 + 3, file_a);
	FILE* file_1_fp = fopen(file_1, "r");
	if (file_1_fp == NULL)
	{
		printf("Couldn't open `%s`, please enter a file that's listed in the database\n\n", file_1);
		free(file_a);
		free(file_1);
		fclose(file_1_fp);
		return;
	}

	// indicates progress
	int file = 1;

	// results
	float* results = malloc(num_files * sizeof(float));
	for (int i = 0; i < num_files; i++)
	{
		// skip if it's the current file
		if (strcmp(file_a, files[i]) == 0)
			continue;

		// progress
		printf("\rComparing files (%d/%d)", file++, num_files - 1);
		fflush(stdout);

		// open and error check the second file
		char* file_b = malloc(strlen(files[i]));
		strcpy(file_b, files[i]);
		char* file_2 = malloc(strlen(file_b) + 3 * sizeof(char));
		file_2[0] = 'd'; file_2[1] = 'b'; file_2[2] = '/';
		strcpy(file_2 + 3, file_b);
		FILE* file_2_fp = fopen(file_2, "r");
		if (file_2_fp == NULL)
		{
			printf("Couldn't open `%s`, aborting because this shouldn't happen...\n", file_2);
			free(file_a);
			free(file_b);
			free(file_1);
			free(file_2);
			fclose(file_1_fp);
			return;
		}

		// a circular array to store words
		char** circle = malloc(SHINGLE_LENGTH * sizeof(char*));
		int* circle_lens = malloc(SHINGLE_LENGTH * sizeof(int));
		int circle_index = 0;

		// variables for file 1
		int sz_unit = BUFSIZ * sizeof(uint64_t);
		uint64_t* f1_shingles = malloc(sz_unit);
		ssize_t f1_shingles_sz = sz_unit;
		int f1_shingles_count = 0;
		int f1_stored_shingles = 0;

		// get words from file 1
		char* word = get_next_word(file_1_fp);
		while (word != NULL)
		{
			// add word to the circle
			circle_lens[circle_index]= strlen(word);
			circle[circle_index] = malloc(circle_lens[circle_index]);
			strncpy(*(circle + circle_index), word, circle_lens[circle_index]);
			circle_index = (circle_index + 1) % SHINGLE_LENGTH;
			f1_shingles_count++;

			// shingle if ready
			if (f1_shingles_count >= SHINGLE_LENGTH)
			{
				// get a string and convert it to a 64-bit number
				char* str = concatenate_circle(circle_index, circle, circle_lens);
				uint64_t hash =  MurmurHash64A(str, strlen(str), seed);

				// possibly increase array size and then store number
				if (f1_shingles_count % BUFSIZ == 0 && f1_shingles_count != 0)
				{
					f1_shingles_sz += sz_unit;
					f1_shingles = realloc(f1_shingles, f1_shingles_sz);
				}
				f1_shingles[f1_stored_shingles++] = hash;
			}

			// get next word
			word = get_next_word(file_1_fp);
		}

		// reset circle
		for (int i = 0; i < SHINGLE_LENGTH; i++)
		{
			free(circle[i]);
			circle_lens[i] = 0;
		}
		circle_index = 0;

		// variables for file 2
		uint64_t* f2_shingles = malloc(sz_unit);
		ssize_t f2_shingles_sz = sz_unit;
		int f2_shingles_count = 0;
		int f2_stored_shingles = 0;

		// get words from file 2
		word = get_next_word(file_2_fp);
		while (word != NULL)
		{
			// add word to circle
			circle_lens[circle_index]= strlen(word);
			circle[circle_index] = malloc(circle_lens[circle_index]);
			strncpy(*(circle + circle_index), word, circle_lens[circle_index]);
			circle_index = (circle_index + 1) % SHINGLE_LENGTH;
			f2_shingles_count++;

			// shingle if ready
			if (f2_shingles_count >= SHINGLE_LENGTH)
			{
				// get a string and convert it to a 64-bit number
				char* str = concatenate_circle(circle_index, circle, circle_lens);
				uint64_t hash =  MurmurHash64A(str, strlen(str), seed);

				// possibly increase array size and then store number
				if (f2_shingles_count % BUFSIZ == 0 && f2_shingles_count != 0)
				{
					f2_shingles_sz += sz_unit;
					f2_shingles = realloc(f2_shingles, f2_shingles_sz);
				}
				f2_shingles[f2_stored_shingles++] = hash;
			}

			// get next words
			word = get_next_word(file_2_fp);
		}

		// reset circle
		for (int i = 0; i < SHINGLE_LENGTH; i++)
		{
			free(circle[i]);
			circle_lens[i] = 0;
		}
		circle_index = 0;

		// permute and compare file similarities
		results[i] = permute_and_compare(file_a, f1_shingles_count, f1_shingles, 
				            			 file_b, f2_shingles_count, f2_shingles);

		// clean up
		free(file_b);
		free(file_2);
		fclose(file_2_fp);
		fseek(file_1_fp, 0, SEEK_SET);   // reset first file position
	}

	// print results
	printf("\n");
	for (int i = 0; i < num_files; i++)
	{
		// skip same file
		if (strcmp(file_a, files[i]) == 0)
			continue;

		// print file and space
		printf("File: %s", files[i]);
		int file_len = strlen(files[i]);
		int len_to_print = 15 - file_len;
		for (int j = 0; j < len_to_print; j++)
			printf(" ");

		// print hashes
		printf("[");
		int percent = results[i] * 100;
		int hashes = percent / 10;
		int j;
		for (j = 0; j < 10; j++)
		{
			if (hashes > 0)
			{
				printf("#");
				hashes--;
			}
			else
				printf(" ");
		}
		printf("]    (%d/%d)\n", (percent / 10), 10);
	}
	printf("\n");

	// clean up
	for (int i = 0; i < num_files; i++)
		free(files[i]);
	free(files);
	free(file_a);
	free(file_1);
	free(results);
}

/*
 *  permute_and_compare
 *  Permuates and compares two sets of numbers, then returns the result (similarity)
 */
float permute_and_compare(char* file_1, int set_1_len, uint64_t* set_1,
         			     char* file_2, int set_2_len, uint64_t* set_2)
{
	// make sure inputs are valid
	assert(file_1 != NULL);
	assert(file_2 != NULL);
	assert(set_1 != NULL);
	assert(set_2 != NULL);

	// 2D arrays
	uint64_t** p_set_1 = malloc(set_1_len * sizeof(uint64_t*));
	uint64_t** p_set_2 = malloc(set_2_len * sizeof(uint64_t*));
	for (int i = 0; i < set_1_len; i++)
	{
		uint64_t* array = malloc(PERMUTATIONS * sizeof(uint64_t));
		p_set_1[i] = array;
	}
	for (int i = 0; i < set_2_len; i++)
	{
		uint64_t* array = malloc(PERMUTATIONS * sizeof(uint64_t));
		p_set_2[i] = array;
	}

	// generate 100 random numbers 
	for (int i = 0; i < set_1_len; i++)
	{
		// use set 1 number as a seed
		srand(set_1[i]);
		for (int j = 0; j < PERMUTATIONS; j++)
			p_set_1[i][j] = rand();
	}
	for (int i = 0; i < set_2_len; i++)
	{
		// use set 2 number as a seed
		srand(set_2[i]);
		for (int j = 0; j < PERMUTATIONS; j++)
			p_set_2[i][j] = rand();
	}

	// compute resemblance
	int matching_mins = 0;
	int calculated_mins = 0;
	uint64_t set_1_min;
	uint64_t set_2_min;
	for (int i = 0; i < PERMUTATIONS; i++)
	{
		// min of permutated set 1
		for (int j = 0; j < set_1_len; j++)
			set_1_min = (j == 0) ? p_set_1[j][i] : ((set_1_min < p_set_1[j][i]) ? set_1_min : p_set_1[j][i]);

		// min of permutated set 2
		for (int j = 0; j < set_2_len; j++)
			set_2_min = (j == 0) ? p_set_2[j][i] : ((set_2_min < p_set_2[j][i]) ? set_2_min : p_set_2[j][i]);

		// see if mins match
		calculated_mins++;
		if (set_1_min == set_2_min)
			matching_mins += 1;
	}

	// calculate resemblance
	float resemblance = (float)matching_mins / (float)PERMUTATIONS;

	// clean up
	for (int i = 0; i < set_1_len; i++)
		free(p_set_1[i]);
	for (int i = 0; i < set_2_len; i++)
		free(p_set_2[i]);
	free(p_set_1);
	free(p_set_2);

	// return result
	return resemblance;
}

/*
 *  get_next_word
 *  Gets the next word from a file
 */
char* get_next_word(FILE* file)
{
	// get words while possible
    char* word = malloc(BUFSIZ * BUFSIZ * sizeof(char));
    int index = 0;
    for (int c = fgetc(file); c != EOF; c = fgetc(file))
    {
        // allow only alphabetical characters and apostrophes or numbers
        if (isalpha(c) || (c == '\'' && index > 0) || isdigit(c))
        {
            // append character to word
            word[index] = c;
            index++;
        }

        // we must have found a whole word
        else if (index > 0)
        {
            // terminate current word
            word[index] = '\0';

            // return next word
            return word;
        }
    }

    // no more words
    return NULL;
}

/*
 * concatenate_circle
 * Concatenates all the words in a circular array
 */
char* concatenate_circle(int head, char** circle, int* circle_lens)     
{
	// get length of string needed
	int len = 0;
	for (int i = 0; i < SHINGLE_LENGTH; i++)
		len += circle_lens[i];

	// make string
	int trav = head;
	int str_index = 0;
	char* str = malloc(len * sizeof(char));
	for (int i = 0; i < SHINGLE_LENGTH; i++)
	{
		strncpy(str + str_index, circle[trav], circle_lens[trav]);
		str_index += circle_lens[trav];
		trav = (trav + 1) % SHINGLE_LENGTH;
	}
	return str;
}

/*
 *  MurmurHash2, 64-bit versions, by Austin Appleby
 *
 *  The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
 *  and endian-ness issues if used across multiple platforms.
 *
 *  64-bit hash for 64-bit platforms
 */
uint64_t MurmurHash64A (const void* key, int len, uint64_t seed)
{
  const uint64_t m = BIG_CONSTANT(0xc6a4a7935bd1e995);
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const uint64_t * data = (const uint64_t *)key;
  const uint64_t * end = data + (len/8);

  while(data != end)
  {
    uint64_t k = *data++;

    k *= m; 
    k ^= k >> r; 
    k *= m; 
    
    h ^= k;
    h *= m; 
  }

  const unsigned char * data2 = (const unsigned char*)data;

  switch(len & 7)
  {
  case 7: h ^= ((uint64_t) data2[6]) << 48;
  case 6: h ^= ((uint64_t) data2[5]) << 40;
  case 5: h ^= ((uint64_t) data2[4]) << 32;
  case 4: h ^= ((uint64_t) data2[3]) << 24;
  case 3: h ^= ((uint64_t) data2[2]) << 16;
  case 2: h ^= ((uint64_t) data2[1]) << 8;
  case 1: h ^= ((uint64_t) data2[0]);
          h *= m;
  };
 
  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
} 