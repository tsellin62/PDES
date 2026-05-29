#include "queues.h"

void init_car(car *c, int id) {
    c->id = id;
    c->sequence = 0;
    c->passenger_count = 0;
    c->state = LOADING;
    c->boarded = 0;
    c->unboarded = 0;
    pthread_mutex_init(&c->lock, NULL);
    pthread_cond_init(&c->board_done, NULL);
    pthread_cond_init(&c->unboard_done, NULL);
}

void init_passenger(passenger *p, int id) {
	p->id = id;
    p->board_signal = 0;
    p->unboard_signal = 0;
	p->seed = id * 12345;
    pthread_mutex_init(&p->lock, NULL);
    pthread_cond_init(&p->can_board, NULL);
    pthread_cond_init(&p->can_unboard, NULL);
}

void clean_car(car *c) {
    pthread_mutex_destroy(&c->lock);
    pthread_cond_destroy(&c->board_done);
    pthread_cond_destroy(&c->unboard_done);
}

void clean_passenger(passenger *p) {
    pthread_mutex_destroy(&p->lock);
    pthread_cond_destroy(&p->can_board);
    pthread_cond_destroy(&p->can_unboard);
}

void explore_park(passenger* p) {
	printf("[Time: ] Passenger %d is exploring the park\n", p->id);
	int explore_time = (rand_r(&p->seed) % 10) + 1;
	sleep(explore_time);
	printf("[Time: ] Passenger %d explored for %d seconds\n", p->id, explore_time);
}

void get_ride_ticket(passenger* p) {}

void enter_ride_queue(passenger* p) {}

void board_car(passenger* p) {}

void unboard_car(passenger* p) {}
