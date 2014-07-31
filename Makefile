# CS 165
# Final Project
# Nicholas Mahlangu

all: clean database

database: database.c 
	gcc -O0 -ggdb3 -g -lreadline -std=c99 database.c -o database

clean:
	rm -f *.o a.out core database