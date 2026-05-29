#ifndef QUEUES_H
#define QUEUES_H
#include <pthread.h>

//loading zone
pthread_mutex_t loading_zone_mutex;
pthread_cond_t  loading_zone_free;

//ride queue and ticket booth
pthread_mutex_t ticket_booth_mutex;
pthread_mutex_t ride_queue_mutex;
pthread_cond_t  ride_queue_not_full;

//park
int park_open;
pthread_mutex_t park_mutex;

typedef enum {
	LOADING,
	RUNNING,
	UNLOADING
} car_state;

typedef struct {
	int passenger_count;
	car_state state;
	int boarded;
	int unboarded;
	pthread_mutex_t lock;
	pthread_cond_t board_done;
	pthread_cond_t unboard_done;
} car;

typedef struct {
	int id;
	int board_signal;
	int unboard_signal;
	unsigned int seed;
	pthread_mutex_t lock;
	pthread_cond_t can_board;
	pthread_cond_t can_unboard;
} passenger;

void init_car(car* c, int id);
void clean_car(car* c);

void init_passenger(passenger* p, int id);
void clean_passenger(passenger* p);

void explore_park(passenger* p);
void get_ride_ticket(passenger* p);
void enter_ride_queue(passenger* p);
void board_car(passenger* p);
void unboard_car(passenger* p);

#endif
