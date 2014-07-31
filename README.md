Plagiarism Database
- Nicholas Mahlangu

How to Compile
- just type make on the command line, a make file is included

How to Test
- Can type in files when running the program
- To test the basic database functionality:
	./database < tests/option1_tests.txt
- To test comparing 2 files RUNS number of times (RUNS is a constant
  defined in database.c, it can be changed):
  	./database < tests/option2_tests.txt
- To test comparing a file against every other file in the database:
	./database < tests/option3_tests.txt

db/ (directory)
- Contains a bunch of random text files for input
- lorem_a.txt is the baseline file
- lorem_b.txt differs from the baseline by 10 words
- lorem_c.txt differs from the baseline by 20 words
- lorem_d.txt differs from the baseline by 30 words
- lorem_e.txt differs from the baseline by 40 words
- lorem_f.txt differs from the baseline by 50 words
- lorem_g.txt is the same as the baseline file

database.c
- Main file
- Need to have the readline library for this (brew install readline)

init
- File that keeps track of all the files in the database
- Add a file to the /db foler and it's name to this list and it will automatically
  be added to the database

MurmurHash2.h
- Header file for the 64-bit hash function MurmurHash2
