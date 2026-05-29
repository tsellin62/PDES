#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queues.h"

//default options
int n = 30;  //number of passenger threads
int c = 4;   //number of car threads
int p = 2;   //capacity per car
int w = 3;   //car waiting period
int r = 2;   //car ride duration
int t = 60;  //park open duration
int j = 10;  //ride queue max size

//park state
int park_open = 1;
pthread_mutex_t park_mutex;

//ticket booth
pthread_mutex_t ticket_booth_mutex;

//ride queue
int ride_queue_size = 0;
pthread_mutex_t ride_queue_mutex;
pthread_cond_t ride_queue_not_full;

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
    pthread_mutex_init(&loading_zone_mutex, NULL);
    pthread_cond_init(&loading_zone_free, NULL);
    pthread_mutex_init(&unload_order_mutex, NULL);
    pthread_cond_init(&my_turn_to_unload, NULL);
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

	car* cars = (car*)malloc(c * sizeof(car));
	passenger* passengers = (passenger*)malloc(n * sizeof(passenger));
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

int is_park_open() {
	pthread_mutex_lock(&park_mutex);
	int open = park_open;
	pthread_mutex_unlock(&park_mutex);
	return open;
}

void passenger_thread(void* arg) {
	passenger* p = (passenger*)arg;

	while (is_park_open()) {
		explore_park(p);
		get_ride_ticket(p);
		enter_ride_queue(p);
		board_car(p);
		unboard_car(p);
	}
	
	return;
}
