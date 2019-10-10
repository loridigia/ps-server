CC=gcc
CFLAGS=-I. -std=gnu99
DEPS = unix.h ../core/core.h ../shared/shared.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: ../main.o ../shared/shared.o unix.o
	$(CC) -o ps_server ../main.o ../unix/unix.o ../shared/shared.o -pthread

.PHONY: clean

clean:
	find .. -type f -name '*.o' -exec rm {} + 
	rm ps_server
