OBJS = queue.o lock.o packetsource.o generators.o crc32.o fingerprint.o
CC = gcc
CFLAGS = -g -Wall -Werror -O3
LIBS = -lm -pthread

all: main test

main: main.o $(OBJS)
	$(CC) $(CFLAGS) -o main main.o $(OBJS) $(LIBS)

test: test.o $(OBJS)
	$(CC) $(CFLAGS) -o test test.o $(OBJS) $(LIBS)

main.o: main.c main.h queue.h lock.h
	$(CC) $(CFLAGS) -c main.c

test.o: test.c test.h
	$(CC) $(CFLAGS) -c test.c

queue.o: queue.c queue.h utils/packetsource.h
	$(CC) $(CFLAGS) -c queue.c

lock.o: lock.c lock.h utils/paddedprim.h
	$(CC) $(CFLAGS) -c lock.c

packetsource.o: utils/packetsource.c utils/packetsource.h
	$(CC) $(CFLAGS) -c utils/packetsource.c

generators.o: utils/generators.c utils/generators.h
	$(CC) $(CFLAGS) -c utils/generators.c

crc32.o: utils/crc32.c utils/crc32.h
	$(CC) $(CFLAGS) -c utils/crc32.c

fingerprint.o: utils/fingerprint.c utils/fingerprint.h
	$(CC) $(CFLAGS) -c utils/fingerprint.c

clean:
	rm -f main test *.o *.txt *~ *#
	@echo "all cleaned up!"
