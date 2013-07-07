.PHONY: clean

CFLAGS = -Wall -Wextra -ggdb

keytest: main.c
	gcc $(CFLAGS) $^ -lcaca -o $@

clean:
	rm -f *.o keytest
