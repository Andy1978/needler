.PHONY: clean

CFLAGS = -Wall -Wextra -ggdb

menutest: menutest.c menu.o
	gcc $(CFLAGS) $^ -lcaca -o $@

menu.o: ../menu.c
	gcc $(CFLAGS) -c $^

clean:
	rm -f *.o menutest
