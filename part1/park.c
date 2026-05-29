#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

//enums for car and passengers
typedef enum {
	LOADING,
	RUNNING,
	UNLOADING
} car_state;

typedef struct {
	int id;
	int passenger_count;
	car_state state;
	int boarded;
	int unboarded;
	int* aboard;
	pthread_mutex_t lock;
	pthread_cond_t board_done;
	pthread_cond_t unboard_done;
} car;

typedef struct {
	int id;
	int board_signal;
	int unboard_signal;
	car* my_car;
	unsigned int seed;
	pthread_mutex_t lock;
	pthread_cond_t can_board;
	pthread_cond_t can_unboard;
} passenger;

//default options
int n = 1;   //number of passenger threads
int c = 1;   //number of car threads
int p = 1;   //capacity per car
int w = 1;   //car waiting period
int r = 1;   //car ride duration
int t = 25;  //park open duration
int j = 1;   //ride queue max size

//passenger/car arrays
car* cars;
passenger* passengers;

//park state
int park_open = 1;
pthread_mutex_t park_mutex;

//ticket booth
pthread_mutex_t ticket_booth_mutex;

//ride queue
int ride_queue_size = 0;
int* ride_queue;
int ride_queue_head = 0;
int ride_queue_tail = 0;
pthread_mutex_t ride_queue_mutex;
pthread_cond_t ride_queue_not_full;
pthread_cond_t passenger_ready;

//loading zone (one car loads at a time)
pthread_mutex_t loading_zone_mutex;
pthread_cond_t loading_zone_free;
int loading_zone_occupied = 0;

//unload ordering
int next_unload_seq = 0;
int current_sequence = 0;
pthread_mutex_t unload_order_mutex;
pthread_cond_t my_turn_to_unload;

void init_globals() {
    pthread_mutex_init(&park_mutex, NULL);
    pthread_mutex_init(&ticket_booth_mutex, NULL);
    pthread_mutex_init(&ride_queue_mutex, NULL);
    pthread_cond_init(&ride_queue_not_full, NULL);
	pthread_cond_init(&passenger_ready, NULL);
    pthread_mutex_init(&loading_zone_mutex, NULL);
    pthread_cond_init(&loading_zone_free, NULL);
    pthread_mutex_init(&unload_order_mutex, NULL);
    pthread_cond_init(&my_turn_to_unload, NULL);
	ride_queue = (int*)malloc(j * sizeof(int));
}

void destroy_globals() {
    pthread_mutex_destroy(&park_mutex);
    pthread_mutex_destroy(&ticket_booth_mutex);
    pthread_mutex_destroy(&ride_queue_mutex);
    pthread_cond_destroy(&ride_queue_not_full);
    pthread_mutex_destroy(&loading_zone_mutex);
    pthread_cond_destroy(&loading_zone_free);
    pthread_mutex_destroy(&unload_order_mutex);
    pthread_cond_destroy(&my_turn_to_unload);
}

//initialize and clean up passengers and cars
void init_car(car *c, int id) {
    c->id = id;
    c->passenger_count = 0;
    c->state = LOADING;
    c->boarded = 0;
    c->unboarded = 0;
	c->aboard = (int*)malloc(p * sizeof(int));
    pthread_mutex_init(&c->lock, NULL);
    pthread_cond_init(&c->board_done, NULL);
    pthread_cond_init(&c->unboard_done, NULL);
}

void init_passenger(passenger *p, int id) {
	p->id = id;
    p->board_signal = 0;
    p->unboard_signal = 0;
	p->seed = id * 12345;
	p->my_car = NULL;
    pthread_mutex_init(&p->lock, NULL);
    pthread_cond_init(&p->can_board, NULL);
    pthread_cond_init(&p->can_unboard, NULL);
}

void clean_car(car *c) {
	free(c->aboard);
    pthread_mutex_destroy(&c->lock);
    pthread_cond_destroy(&c->board_done);
    pthread_cond_destroy(&c->unboard_done);
}

void clean_passenger(passenger *p) {
    pthread_mutex_destroy(&p->lock);
    pthread_cond_destroy(&p->can_board);
    pthread_cond_destroy(&p->can_unboard);
}

//other functions
int get_time() {
	static time_t start = 0;
	if (start == 0) {
		start = time(NULL);
	}
	return (int)(time(NULL) - start);
}

int is_park_open() {
	pthread_mutex_lock(&park_mutex);
	int open = park_open;
	pthread_mutex_unlock(&park_mutex);
	return open;
}

void enqueue(int passenger_id) {
	ride_queue[ride_queue_tail] = passenger_id;
	ride_queue_tail = (ride_queue_tail + 1) % j;
	ride_queue_size++;
}

int dequeue() {
	int id = ride_queue[ride_queue_head];
	ride_queue_head = (ride_queue_head - 1) % j;
	ride_queue_size--;
	return id;
}

//passenger functions
void explore_park(passenger* p) {
	printf("[Time: %d] Passenger %d is exploring the park\n", get_time(), p->id);
	int explore_time = (rand_r(&p->seed) % 10) + 1;
	sleep(explore_time);
	printf("[Time: %d] Passenger %d explored for %d seconds\n", get_time(), p->id, explore_time);
}

int get_ride_ticket(passenger* p) {
	pthread_mutex_lock(&ticket_booth_mutex);

	//if ride is full, wait
	while (ride_queue_size >= j && is_park_open()) {
		pthread_cond_wait(&ride_queue_not_full, &ticket_booth_mutex);
	}

	//if park closes while waiting, exit
	if (!is_park_open()) {
		pthread_mutex_unlock(&ticket_booth_mutex);
		return 0;
	}
	
	ride_queue_size++;
	printf("[Time: %d] Passenger %d acquired a ticket\n", get_time(), p->id);
	pthread_mutex_unlock(&ticket_booth_mutex);
	return 1;
}

void enter_ride_queue(passenger* p) {
	printf("[Time: %d] Passenger %d has entered the ride queue\n", get_time(), p->id);
	pthread_mutex_lock(&ride_queue_mutex);
	enqueue(p->id);
	pthread_cond_signal(&passenger_ready);
	pthread_mutex_unlock(&ride_queue_mutex);

	pthread_mutex_lock(&p->lock);
	while (!p->board_signal) {
		pthread_cond_wait(&p->can_board, &p->lock);
	}

	p->board_signal = 0;
	pthread_mutex_unlock(&p->lock);
	printf("[Time: %d] Passenger %d is boarding\n", get_time(), p->id);
}

void board_car(passenger *p) {
    pthread_mutex_lock(&p->my_car->lock);
    p->my_car->boarded++;
    //printf("[Time: ] Passenger %d boarded, total boarded: %d\n", p->id, p->my_car->boarded);

    pthread_cond_signal(&p->my_car->board_done);
    pthread_mutex_unlock(&p->my_car->lock);
}

void unboard_car(passenger *p) {
    pthread_mutex_lock(&p->lock);

    while (!p->unboard_signal) {
        pthread_cond_wait(&p->can_unboard, &p->lock);
    }

	//reset for next ride
    p->unboard_signal = 0;
    pthread_mutex_unlock(&p->lock);

    //tell the car this passenger has unboarded
    pthread_mutex_lock(&p->my_car->lock);
    p->my_car->unboarded++;
    printf("[Time: %d] Passenger %d unboarded\n", get_time(), p->id);
    pthread_cond_signal(&p->my_car->unboard_done);
    pthread_mutex_unlock(&p->my_car->lock);

    //decrement ride queue and signal ticket booth
    pthread_mutex_lock(&ride_queue_mutex);
    ride_queue_size--;
    pthread_cond_signal(&ride_queue_not_full);
    pthread_mutex_unlock(&ride_queue_mutex);

	//detach from car
    p->my_car = NULL;
}

//car functions
void load(car *c) {
    //get loading zone
    pthread_mutex_lock(&loading_zone_mutex);
    while (loading_zone_occupied) {
        pthread_cond_wait(&loading_zone_free, &loading_zone_mutex);
    }
    loading_zone_occupied = 1;
    pthread_mutex_unlock(&loading_zone_mutex);

    pthread_mutex_lock(&c->lock);
    c->state = LOADING;
    c->boarded = 0;
    c->passenger_count = 0;
    pthread_mutex_unlock(&c->lock);

    //wait until at least one passenger is in the queue
    pthread_mutex_lock(&ride_queue_mutex);
    while (ride_queue_size == 0) {
        pthread_cond_wait(&passenger_ready, &ride_queue_mutex);
    }

    //signal first passenger to board
    int pid = dequeue();
    pthread_mutex_unlock(&ride_queue_mutex);

    passenger *first = &passengers[pid];
    first->my_car = c;
	c->aboard[c->boarded] = first->id;
    pthread_mutex_lock(&first->lock);
    first->board_signal = 1;
    pthread_cond_signal(&first->can_board);
    pthread_mutex_unlock(&first->lock);

    //wait for boarding, keep loading until full or timeout
    pthread_mutex_lock(&c->lock);
    while (c->boarded < p) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += w;

        int result = pthread_cond_timedwait(&c->board_done, &c->lock, &ts);

        if (result == ETIMEDOUT) {
            if (c->boarded > 0) {
                break;
            }
        } else {
            //a passenger boarded, signal next if queue has more
            pthread_mutex_lock(&ride_queue_mutex);
            if (ride_queue_size > 0 && c->boarded < p) {
                int next_pid = dequeue();
                pthread_mutex_unlock(&ride_queue_mutex);

                passenger *next = &passengers[next_pid];
                next->my_car = c;
				c->aboard[c->boarded] = next->id;
                pthread_mutex_lock(&next->lock);
                next->board_signal = 1;
                pthread_cond_signal(&next->can_board);
                pthread_mutex_unlock(&next->lock);
            } else {
                pthread_mutex_unlock(&ride_queue_mutex);
            }
        }
    }

    c->passenger_count = c->boarded;
	if (c->passenger_count == p) {
		printf("[Time: %d] Car %d is full with %d passengers\n", get_time(), c->id, c->passenger_count);
	}
    printf("[Time: %d] Car %d has departed to ride\n", get_time(), c->id);
    pthread_mutex_unlock(&c->lock);
}

void run(car* c) {
	pthread_mutex_lock(&c->lock);
	c->state = RUNNING;
	pthread_mutex_unlock(&c->lock);
	sleep(r);
	printf("[Time: %d] Car %d has returned from the ride\n", get_time(), c->id);
}

void unload(car* c) {
	pthread_mutex_lock(&c->lock);
	c->state = UNLOADING;
	c->unboarded = 0;
	printf("[Time: %d] Car %d has invoked unload()\n", get_time(), c->id);

	//signal each passenger to unboard
	for (int i = 0; i < c->passenger_count; i++) {
		passenger* p = &passengers[c->aboard[i]];
		pthread_mutex_lock(&p->lock);
		p->unboard_signal = 1;
		pthread_cond_signal(&p->can_unboard);
		pthread_mutex_unlock(&p->lock);
	}

	//wait until unboarding complete
	while (c->unboarded < c->passenger_count) {
		pthread_cond_wait(&c->unboard_done, &c->lock);
	}

	//release loading zone
	pthread_mutex_lock(&loading_zone_mutex);
	loading_zone_occupied = 0;
	pthread_cond_signal(&loading_zone_free);
	pthread_mutex_unlock(&loading_zone_mutex);
}

//thread functions
void* passenger_thread(void* arg) {
	passenger* p = (passenger*)arg;

	while (is_park_open()) {
		explore_park(p);
		if(!get_ride_ticket(p)) {
			continue;
		}
		enter_ride_queue(p);
		board_car(p);
		unboard_car(p);
	}	
	return NULL;
}

void* car_thread(void* arg) {
	car* c = (car*)arg;

	while(is_park_open()) {
		load(c);
		run(c);
		unload(c);
	}

	return NULL;
}

//main
int main(int argc, char* argv[]) {	
	//parse flags
	for (int i = 1; i < argc;) {
		if (strcmp(argv[i], "-n") == 0) {
			n = atoi(argv[i + 1]);
			i += 2;
			continue;
		}
		else if (strcmp(argv[i], "-c") == 0) {
			c = atoi(argv[i + 1]);
			i += 2;
			continue;
		}
		else if (strcmp(argv[i], "-p") == 0) {
			p = atoi(argv[i + 1]);
			i += 2;
			continue;
		}
		else if (strcmp(argv[i], "-w") == 0) {
			w = atoi(argv[i + 1]);
			i += 2;
			continue;
		}
		else if (strcmp(argv[i], "-r") == 0) {
			r = atoi(argv[i + 1]);
			i += 2;
			continue;
		}
		else if (strcmp(argv[i], "-t") == 0) {
			t = atoi(argv[i + 1]);
			i += 2;
			continue;
		}
		else if (strcmp(argv[i], "-j") == 0) {
			j = atoi(argv[i + 1]);
			i += 2;
			continue;
		}
		else if (strcmp(argv[i], "-h") == 0) {
			printf("Usage: ./park [OPTIONS]\n");
			return 0;
		}
		else {
			printf("Error with flags, try again with correct usage\n");
			return 0;
		}

	}

	printf("n: %d, c: %d, p: %d, w: %d, r: %d, t: %d, j: %d\n", n, c, p, w, r, t, j);

	init_globals();

	cars = (car*)malloc(c * sizeof(car));
	passengers = (passenger*)malloc(n * sizeof(passenger));
	pthread_t* car_threads = (pthread_t*)malloc(c * sizeof(pthread_t));
	pthread_t* passenger_threads = (pthread_t*)malloc(n * sizeof(pthread_t));

	for (int i = 0; i < c; i++) {
		init_car(&cars[i], i);
	}
	for (int i = 0; i < n; i++) {
		init_passenger(&passengers[i], i);
	}
	
	for (int i = 0; i < c; i++) {
		pthread_create(&car_threads[i], NULL, car_thread, &cars[i]);
	}
	for (int i = 0; i < n; i++) {
		pthread_create(&passenger_threads[i], NULL, passenger_thread, &passengers[i]);
	}

	//run park for t
	sleep(t);

	//close park
	pthread_mutex_lock(&park_mutex);
	park_open = 0;
	pthread_mutex_unlock(&park_mutex);

	//wait for threads to finish
	for (int i = 0; i < c; i++) {
		pthread_join(car_threads[i], NULL);
	}
	for (int i = 0; i < n; i++) {
		pthread_join(passenger_threads[i], NULL);
	}

	for (int i = 0; i < c; i++) {
		clean_car(&cars[i]);
	}
	for (int i = 0; i < n; i++) {
		clean_passenger(&passengers[i]);
	}
	destroy_globals();
	free(cars);
	free(passengers);
	free(car_threads);
	free(passenger_threads);
	
	return 0;
}
