#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <random>
#include <algorithm>
#include <fstream>
#include "../../controller/ApplicationMonitor.h"

#define N 50000
#define T 25
#define K 3
#define ERR

int flips=0;
#define NTHREADS 4
int arg[NTHREADS];
pthread_t threads[NTHREADS];
pthread_attr_t attr;

struct point{
	int x,y;
	char c;
	};

struct arg_t{
	int tid;
	int start,end;
	int rcount,gcount,bcount;
	int rx,ry,gx,gy,bx,by;
};

int precisionLevel;

struct point space[N];
struct point means[T];
struct point centroids[K];

void *ethread (void *arg);
void dataGen(point*, int);
float dist(struct point p1,struct point p2);
void kmeans_series(void);
void kmeansApxLevel_series(void);
void kmeans_Parallel(void);
void kmeansApxLevel_Parallel(void);
void *kmeans_P (void *arg);
void InitData(void);
// void FlushgVars (void);
// char ret_status [10] [100];

int main (int argc, char **argv)
{
    pthread_t tid [10];
    int i, r;
    void *status;
    char buffer [128];

		int times;
		times = atoi(argv[1]);
		int precisionLevel;
		float accLoss=0;
		std::ofstream precLog("precLog.txt", std::ios::out);
		data_t *data; //pointer to the shared memory to communicate with the monitor
		data = monitorAttach(argv[0], 8, 12, false, true, 4);
		dataGen(space, N);
		dataGen(means, T);
		dataGen(centroids, K);
		centroids[0].c = 'r';
		centroids[1].c = 'g';
		centroids[2].c = 'b';

		for(int i=0; i<times; i++){
			precisionLevel = usePrecisionLevel(data);  // use the level of approximation you need.
			dataGen(space, N);
			dataGen(centroids, K);
			centroids[0].c = 'r';
	    centroids[1].c = 'g';
	    centroids[2].c = 'b';
	    if(precisionLevel ==0){
				// kmeans_series();
				kmeans_Parallel();
			}
	    else{
	    	kmeansApxLevel_Parallel();
			}
			monitorTick(data);
	    accLoss+=precisionLevel;
	    precLog << precisionLevel << std::endl;
	  }
		monitorDetach(data);
		precLog.close();

		std::cout << "-------------------------------- " << std::endl;
		std::cout << "Relaxed convergence for K-Means = " << accLoss/times << std::endl;
		std::cout << "-------------------------------- " << std::endl;
}

void InitData(void)
{
	dataGen(space, N);
	dataGen(means, T);
	dataGen(centroids, K);
	centroids[0].c = 'r';
	centroids[1].c = 'g';
	centroids[2].c = 'b';
}

void kmeans_series(void)
{
//int flips;
		do{
		flips = 0;
		int rcount =0,gcount=0,bcount =0;
		int rx,ry,gx,gy,bx,by;
		rx = ry = bx = by = gx = gy = 0;

	for(int i=0; i<N; i++){
	 	if(space[i].c == 'r'){
	  	rcount++;
	  	rx += space[i].x;
	  	ry += space[i].y;
	 	}
	 	if(space[i].c == 'g'){
	  	gcount++;
	  	gx += space[i].x;
	  	gy += space[i].y;
	 	}
	 	if(space[i].c == 'b'){
	  	bcount++;
	  	bx += space[i].x;
	  	by += space[i].y;
	 	}
	}

	// std::cout << "rcount: " << rcount << std::endl;

	// std::cout << "gx: " << gx << " rx: " << rx << " bx: " << bx << std::endl;
	// std::cout << "gcount: " << gcount << " rcount: " << rcount << " bcount: " << bcount << std::endl;
	for(int i=0; i<K; i++){
		if(i==0 && rcount!=0){
	 		centroids[i].x = rx/rcount;
	 		centroids[i].y = ry/rcount;
		}
		else if(i==1 && gcount!=0){
	 		centroids[i].x = gx/gcount;
	 		centroids[i].y = gy/gcount;
		}
		else if(i==2 && bcount!=0){
	 		centroids[i].x = bx/bcount;
	 		centroids[i].y = by/bcount;
		}
	}
	 char cc;
	 for(int i=0; i<N; i++){
		 float d, min=1000000;
		 for(int j=0; j<K; j++){
				d = dist(space[i], centroids[j]);
				if(d < min){
					min = d;
					cc = centroids[j].c;
				}
			}
			if(space[i].c != cc){
				space[i].c = cc;
				flips++;
			}
		}
		std::cout << "Flips = " << flips << std::endl;
	}while(flips!=0);
}

//appx version with configurable level of accuracy
void kmeansApxLevel_series()
{
//int flips;
int flipVector[27] = {60, 48, 76, 57, 24, 40, 57, 39, 96, 76, 55, 80, 63, 111, 190, 127, 60, 223, 200, 529, 525, 876, 442, 1737, 4631, 8491, 33154};
int index = precisionLevel/5-1;
if(index < 0)
	index =0;
if(index > 27)
	index = 27;
int relax = flipVector[index];
//  int relax = N*l/500; //N* (l/5) / 100 --l/5 % of N to be relaxed
		do{
	  flips = 0;
		int rcount =0,gcount=0,bcount =0;
		int rx,ry,gx,gy,bx,by;
		rx = ry = bx = by = gx = gy = 0;

		for(int i=0; i<N; i++){
	 	 if(space[i].c == 'r'){
	 		 rcount++;
	 		 rx += space[i].x;
	 		 ry += space[i].y;
	 	 }
	 	 if(space[i].c == 'g'){
	 		 gcount++;
	 		 gx += space[i].x;
	 		 gy += space[i].y;
	 	 }
	 	 if(space[i].c == 'b'){
	 		 bcount++;
	 		 bx += space[i].x;
	 		 by += space[i].y;
	 	 }
	  }
	 for(int i=0; i<K; i++){
		 if(i==0){
			 centroids[i].x = rx/rcount;
			 centroids[i].y = ry/rcount;
		 }
		 else if(i==1){
			 centroids[i].x = gx/gcount;
			 centroids[i].y = gy/gcount;
		 }
		 else{
			 centroids[i].x = bx/bcount;
			 centroids[i].y = by/bcount;
		 }
	 }
	char cc;
	for(int i=0; i<N; i++){
		float d, min=1000000;

		for(int j=0; j<K; j++){
 			d = dist(space[i], centroids[j]);
			if(d < min){
				min = d;
				cc = centroids[j].c;
				}
		}
		if(space[i].c != cc){
			space[i].c = cc;
			flips++;
		}
	}
	std::cout << "Flips = " << flips << std::endl;

}while(flips != relax);
//}while(flips > relax);
}

void kmeans_Parallel(void)
{
	arg_t* args_list = new arg_t[NTHREADS];
	pthread_t* threads_list = new pthread_t[NTHREADS];
	int thread_no [NTHREADS];
	int BatchSize = N/NTHREADS;
	int i,j;

	for (int i=0; i<NTHREADS; i++){
		args_list[i].rcount = 0;
		args_list[i].gcount = 0;
		args_list[i].bcount = 0;
		args_list[i].gx = 0;
		args_list[i].gy = 0;
		args_list[i].rx = 0;
		args_list[i].ry = 0;
		args_list[i].bx = 0;
		args_list[i].by = 0;
	}

	do{
		flips = 0;
		int rcount =0,gcount=0,bcount =0;
		int rx = 0,ry = 0,gx = 0,gy = 0,bx = 0,by = 0;
		for(i = 0, j = 0; i < N; i+=BatchSize, j+=1)
		{
			 	 args_list[j].tid = j;
				 args_list[j].start = i;
				 args_list[j].end = std::min(i + BatchSize - 1, N - 1);
				 pthread_create (&threads_list[j], NULL, kmeans_P, (void*)&args_list[j]);
		}
		// Wait for threads to terminate
		for(int i=0; i<NTHREADS; i++){
			pthread_join(threads_list[i], NULL);
	    rcount += args_list[i].rcount;
	    gcount += args_list[i].gcount;
			bcount += args_list[i].bcount;
	    gx += args_list[i].gx;
	    gy += args_list[i].gy;
			rx += args_list[i].rx;
	    ry += args_list[i].ry;
			bx += args_list[i].bx;
			by += args_list[i].by;
	  }

		for(int i=0; i<K; i++){
			if(i==0 && rcount!=0){
		 		centroids[i].x = rx/rcount;
		 		centroids[i].y = ry/rcount;
			}
			else if(i==1 && gcount!=0){
		 		centroids[i].x = gx/gcount;
		 		centroids[i].y = gy/gcount;
			}
			else if(i==2 && bcount!=0){
		 		centroids[i].x = bx/bcount;
		 		centroids[i].y = by/bcount;
			}
		}
		char cc;
		for(int i=0; i<N; i++){
		 float d, min=1000000;
		 for(int j=0; j<K; j++){
				d = dist(space[i], centroids[j]);
				if(d < min){
					min = d;
					cc = centroids[j].c;
				}
			}
			if(space[i].c != cc){
				space[i].c = cc;
				flips++;
			}
		}
	std::cout << "Flips = " << flips << std::endl;
	}while(flips!=0);
}

void kmeansApxLevel_Parallel(void)
{
	arg_t* args_list = new arg_t[NTHREADS];
	pthread_t* threads_list = new pthread_t[NTHREADS];
	int thread_no [NTHREADS];
	int BatchSize = N/NTHREADS;
	int i,j;

	for (int i=0; i<NTHREADS; i++){
		args_list[i].rcount = 0;
		args_list[i].gcount = 0;
		args_list[i].bcount = 0;
		args_list[i].gx = 0;
		args_list[i].gy = 0;
		args_list[i].rx = 0;
		args_list[i].ry = 0;
		args_list[i].bx = 0;
		args_list[i].by = 0;
	}

	int flipVector[23] = {45, 39, 27, 47, 44, 57, 29, 56, 59, 43, 85, 57, 83, 102, 92, 153, 219, 267, 400, 644, 1225, 2852, 33056 };
	int index = precisionLevel/5-1;
	if(index < 0)
		index =0;
	if(index > 19)
		index = 19;
	int relax = flipVector[index];

	do{
		flips = 0;
		int rcount =0,gcount=0,bcount =0;
		int rx = 0,ry = 0,gx = 0,gy = 0,bx = 0,by = 0;
		for(i = 0, j = 0; i < N; i+=BatchSize, j+=1)
		{
			 	 args_list[j].tid = j;
				 args_list[j].start = i;
				 args_list[j].end = std::min(i + BatchSize - 1, N - 1);
				 pthread_create (&threads_list[j], NULL, kmeans_P, (void*)&args_list[j]);
		}
		// Wait for threads to terminate
		for(int i=0; i<NTHREADS; i++){
			pthread_join(threads_list[i], NULL);
	    rcount += args_list[i].rcount;
	    gcount += args_list[i].gcount;
			bcount += args_list[i].bcount;
	    gx += args_list[i].gx;
	    gy += args_list[i].gy;
			rx += args_list[i].rx;
	    ry += args_list[i].ry;
			bx += args_list[i].bx;
			by += args_list[i].by;
	  }

		for(int i=0; i<K; i++){
			if(i==0 && rcount!=0){
		 		centroids[i].x = rx/rcount;
		 		centroids[i].y = ry/rcount;
			}
			else if(i==1 && gcount!=0){
		 		centroids[i].x = gx/gcount;
		 		centroids[i].y = gy/gcount;
			}
			else if(i==2 && bcount!=0){
		 		centroids[i].x = bx/bcount;
		 		centroids[i].y = by/bcount;
			}
		}
		char cc;
		for(int i=0; i<N; i++){
		 float d, min=1000000;
		 for(int j=0; j<K; j++){
				d = dist(space[i], centroids[j]);
				if(d < min){
					min = d;
					cc = centroids[j].c;
				}
			}
			if(space[i].c != cc){
				space[i].c = cc;
				flips++;
			}
		}
	std::cout << "Flips = " << flips << std::endl;
	}while(flips != relax);
}

// ethread: example thread
void *kmeans_P (void *arg)
{
    arg_t* info = (arg_t*) arg;
		int tid = info->tid;
		int start = info->start;
		int end = info->end;
		// std::cout << "tid: " << tid << " start: " << start << " end: " << end << std::endl;

		// say hello and terminate
		for(int i = start; i < end; i++){
			if(space[i].c == 'r'){
				info->rcount++;
				info->rx += space[i].x;
				info->ry += space[i].y;
			}
			if(space[i].c == 'g'){
				info->gcount++;
				info->gx += space[i].x;
				info->gy += space[i].y;
			}
			if(space[i].c == 'b'){
				info->bcount++;
				info->bx += space[i].x;
				info->by += space[i].y;
			}
		}
    // pthread_exit (ret_status [tid]);
		return 0;
}

void printPoint(point p)
{
	std::cout << "Point: (" << p.x << "," << p.y << ")--" << p.c << std::endl;
}

void dataGen(point* train, int s)
{
	int p;
	int seed = 7;
	std::mt19937 rand(seed);
	for(int i=0; i<s; i++){
		p = rand()%3;
		if(p==0){train[i].c = 'r';}
		else if(p==1){ train[i].c = 'g';}
		else{train[i].c = 'b';}
		train[i].x = rand()%1000;
		train[i].y = rand()%600;
	}
}

float dist(struct point p1, struct point p2)
{
	float Dist = sqrt((p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y));
	return Dist;
}
