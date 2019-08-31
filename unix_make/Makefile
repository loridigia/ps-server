CC=gcc
CFLAGS=-I.
DEPS = ../unix/unix.h ../core/core.h ../shared/shared.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: ../main.o ../unix/unix.o ../shared/shared.o ../test.o
	$(CC) -o ps_server ../main.o ../unix/unix.o ../shared/shared.o
	$(CC) -o ps_test ../test.o -lcurl

.PHONY: clean

clean:
	find .. -type f -name '*.o' -exec rm {} + 
	rm ps_server ps_test
