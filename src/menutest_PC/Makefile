.PHONY: clean

CFLAGS = -Wall -Wextra -ggdb

menutest: menutest.c menu.o keymapping.o
	gcc $(CFLAGS) $^ -lcaca -o $@

menu.o: ../menu.c
	gcc $(CFLAGS) -c $^

keymapping.o: ../keymapping.c
	gcc $(CFLAGS) -c $^

clean:
	rm -f *.o menutest
