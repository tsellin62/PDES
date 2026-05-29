CC = gcc
CFLAGS = -Wall -Wextra

park: park.o queues.o
	$(CC) $(CFLAGS) -o park park.o queues.o

park.o: park.c
	$(CC) $(CFLAGS) -c park.c

queues.o: queues.c
	$(CC) $(CFLAGS) -c queues.c

clean:
	rm -f park *.o
