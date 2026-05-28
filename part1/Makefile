CC = gcc
CFLAGS = -Wall -Wextra

park: park.o
	$(CC) $(CFLAGS) -o park park.o

park.o: park.c
	$(CC) $(CFLAGS) -c park.c

clean:
	rm -f park *.o
