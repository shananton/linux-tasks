malloc.so: malloc.o
	gcc -shared -o malloc.so malloc.o

malloc.o: malloc.c
	gcc -fPIC -c -o malloc.o malloc.c

clean:
	rm -f *.o *.so

.PHONY: clean
