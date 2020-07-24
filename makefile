OBJS = queue.o lock.o packetsource.o generators.o crc32.o fingerprint.o
CC = gcc
CFLAGS = -g -Wall -Werror -O3
LIBS = -lm -lrt -pthread

all: main test

main: main.o $(OBJS)
	$(CC) $(CFLAGS) -o main main.o $(OBJS) $(LIBS)

test: test.o $(OBJS)
	$(CC) $(CFLAGS) -o test test.o $(OBJS) $(LIBS)

main.o: src/main.c src/main.h src/queue.h src/lock.h
	$(CC) $(CFLAGS) -c src/main.c

test.o: src/test.c src/test.h
	$(CC) $(CFLAGS) -c src/test.c

queue.o: src/queue.c src/queue.h src/utils/packetsource.h
	$(CC) $(CFLAGS) -c src/queue.c

lock.o: src/lock.c src/lock.h src/utils/paddedprim.h
	$(CC) $(CFLAGS) -c src/lock.c

packetsource.o: src/utils/packetsource.c src/utils/packetsource.h
	$(CC) $(CFLAGS) -c src/utils/packetsource.c

generators.o: src/utils/generators.c src/utils/generators.h
	$(CC) $(CFLAGS) -c src/utils/generators.c

crc32.o: src/utils/crc32.c src/utils/crc32.h
	$(CC) $(CFLAGS) -c src/utils/crc32.c

fingerprint.o: src/utils/fingerprint.c src/utils/fingerprint.h
	$(CC) $(CFLAGS) -c src/utils/fingerprint.c

clean:
	rm -f main test *.o *.txt *~ *#
	@echo "all cleaned up!"
