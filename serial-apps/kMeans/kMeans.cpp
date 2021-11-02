#include <iostream>
#include <random>
#include <algorithm>
#include <fstream>

#define CONTROLLER_ATTACHED 0

#ifdef CONTROLLER_ATTACHED
#include "../../controller/ApplicationMonitor.h"
#endif

#define N 50000
#define T 25
#define k 3
#define ERR

struct point{
	int x,y;
	char c;
	};

void dataGen(point*, int);
float dist(struct point p1,struct point p2);
void kmeans(point*, point*);
void kmeansApx(point*, point*);
void kmeansApxLevel(point*, point*, int);

//full scale perf = 10.4

int main(int argc, char** argv)
{
	int times;
	times = atoi(argv[1]);
	int precisionLevel;
	float accLoss=0;
	std::ofstream precLog("precLog.txt", std::ios::out);
	#ifdef CONTROLLER_ATTACHED
  data_t *data; //pointer to the shared memory to communicate with the monitor
	data = monitorAttach(argv[0], 8, 12, false);
	#endif

	struct point space[N];
  struct point means[T];
  struct point centroids[k];
  dataGen(space, N);
	dataGen(means, T);
	dataGen(centroids, k);
	centroids[0].c = 'r';
	centroids[1].c = 'g';
	centroids[2].c = 'b';

	for(int i=0; i<times; i++){
		#ifdef CONTROLLER_ATTACHED
		precisionLevel = usePrecisionLevel(data);
		#else
		precisionLevel = 5 // use the level of approximation you need.
		#endif
		dataGen(space, N);
		dataGen(centroids, k);
		centroids[0].c = 'r';
	  centroids[1].c = 'g';
	  centroids[2].c = 'b';
    if(precisionLevel ==0)
      kmeans(space, centroids);
		else
      kmeansApxLevel(space, centroids, precisionLevel);
		#ifdef CONTROLLER_ATTACHED
    monitorTick(data);
		#endif
		accLoss+=precisionLevel;
		precLog << precisionLevel << std::endl;
  }
	#ifdef CONTROLLER_ATTACHED
  monitorDetach(data);
	#endif
	precLog.close();

	std::cout << "-------------------------------- " << std::endl;
	std::cout << "Relaxed convergence for K-Means = " << accLoss/times << std::endl;
	std::cout << "-------------------------------- " << std::endl;
return 0;
}

//accureate version
void kmeans(point* space, point* centroids)
{
int flips;
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
	 for(int i=0; i<k; i++){
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
			for(int j=0; j<k; j++){
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
	//std::cout << "Flips = " << flips << std::endl;
	}while(flips!=0);
}

//default appx version

void kmeansApx(point* space, point* centroids)
{
int flips;
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
	 for(int i=0; i<k; i++){
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

			for(int j=0; j<k; j++){
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
	//std::cout << "Flips = " << flips << std::endl;
}while(flips > (N*5/100));
}

//appx version with configurable level of accuracy
void kmeansApxLevel(point* space, point* centroids, int l)
{
int flips;
int flipVector[19] = {60,82,63,74,141,155,200,270,328, 449, 537, 691, 889, 1018, 1031, 1101, 1390, 3650 };
int index = l/5-1;
if(index < 0)
	index =0;
if(index > 19)
	index = 19;
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
	 for(int i=0; i<k; i++){
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

			for(int j=0; j<k; j++){
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
//	std::cout << "Flips = " << flips << std::endl;

}while(flips != relax);
//}while(flips > relax);


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

		if(p==0)
		  train[i].c = 'r';
		else if(p==1)
		  train[i].c = 'g';
		else
		  train[i].c = 'b';
		train[i].x = rand()%1000;
		train[i].y = rand()%600;

	}
}

float dist(struct point p1, struct point p2)
{
	float Dist = sqrt((p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y));
	return Dist;
}
