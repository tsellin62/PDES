CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -lpthread

park: park.o
	$(CC) $(CFLAGS) -o park park.o

park.o: park.c
	$(CC) $(CFLAGS) -c park.c

clean:
	rm -f park *.o
