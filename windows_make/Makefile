CC=gcc
CFLAGS=-I. -std=gnu99
DEPS = ../win/win.h ../core/core.h ../shared/shared.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: ../main.o ../win/win.o ../shared/shared.o
	$(CC) -o ps_server ../main.o ../win/win.o ../shared/shared.o -lws2_32

.PHONY: clean

clean:
	find .. -type f -name '*.o' -exec rm {} +
	rm ps_server ps_test
