main: main.c
	gcc -Wall -Wextra -Werror -O2 -o main main.c

clean:
	rm -f main

.PHONY: clean
