#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
	//default options
	int n = 30;  //number of passenger threads
	int c = 4;   //number of car threads
	int p = 2;   //capacity per car
	int w = 3;   //car waiting period
	int r = 2;   //car ride duration
	int t = 60;  //park open duration
	int j = 10;  //ride queue max size
	
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
}
