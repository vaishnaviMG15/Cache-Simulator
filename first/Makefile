

all: first

first: first.o
	gcc -o first first.o -Werror -Wall -fsanitize=address -lm

clean: 
	rm -rf first.o first


