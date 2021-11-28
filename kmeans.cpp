#include <iostream>
#include <random>
#include <algorithm>
#include <fstream>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <cstring>
using namespace std::chrono;

// #define CONTROLLER_ATTACHED

#ifdef CONTROLLER_ATTACHED
#include "../../controller/ApplicationMonitor.h"
#endif

#define N 50000
#define T 25
#define k 3
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

int rx0,ry0,gx0,gy0,bx0,by0,rcount0,gcount0,bcount0;
int rx1,ry1,gx1,gy1,bx1,by1,rcount1,gcount1,bcount1;
int rx2,ry2,gx2,gy2,bx2,by2,rcount2,gcount2,bcount2;
int rx3,ry3,gx3,gy3,bx3,by3,rcount3,gcount3,bcount3;

struct point space[N];
struct point means[T];
struct point centroids[k];
    
struct point space1[N];
struct point means1[T];
struct point centroids1[k];
    
void kmeans_series(point*, point*, int);
// void kmeansApxLevel_series(point*, point*);

void kmeans_Parallel(point*, point*, int);
// void kmeansApxLevel_Parallel(point*, point*);

void* kmeans_P0(void *tid);
void* kmeans_P1(void *tid);
void* kmeans_P2(void *tid);
void* kmeans_P3(void *tid);

void dataGen(point*, int);
float dist(struct point p1,struct point p2);
void FlushgVars (void);

//full scale perf = 10.4

int main(int argc, char** argv)
{
	int times;
	times = 10;//atoi(argv[1]);
	float accLoss=0;
	std::ofstream precLog("precLog.txt", std::ios::out);
	int precisionLevel;

#ifdef CONTROLLER_ATTACHED

	for(int i=0; i<times; i++){
	    FlushgVars();
	    precisionLevel = usePrecisionLevel(data);
		dataGen(space, N);
		dataGen(centroids, k);
		centroids[0].c = 'r';
	    centroids[1].c = 'g';
	    centroids[2].c = 'b';
    if(precisionLevel ==0)
        kmeans_series();
	else
        kmeansApxLevel_series();
		accLoss+=precisionLevel;
		precLog << precisionLevel << std::endl;
  }
	precLog.close();

	std::cout << "-------------------------------- " << std::endl;
	std::cout << "Relaxed convergence for K-Means = " << accLoss/times << std::endl;
	std::cout << "-------------------------------- " << std::endl;
#else

	for(int i=0; i<times; i++){
		precisionLevel = i*10; // use the level of approximation you need.
		FlushgVars();
		dataGen(space, N);
		dataGen(centroids, k);
		centroids[0].c = 'r';
    	centroids[1].c = 'g';
    	centroids[2].c = 'b';
    	// We need same dataset for Serial and Parallel dataset
        memcpy(space1, space, sizeof(space));
        memcpy(centroids1, centroids, sizeof(centroids));
        
        // Get starting timepoint
        start = high_resolution_clock::now();
          
        // Call the function, here sort()
        kmeans_Parallel(space, centroids, precisionLevel);          
        // Get ending timepoint
        stop = high_resolution_clock::now();
          
        // Get duration. Substart timepoints to 
        // get durarion. To cast it to proper unit
        // use duration cast method
        duration = duration_cast<microseconds>(stop - start);
          
        std::cout << duration.count() << " us" << std::endl;
        std::cout << "P= " << precisionLevel << std::endl;
                 
        // Get starting timepoint
        auto start = high_resolution_clock::now();
          
        // Call the function, here sort()
        kmeans_series(space1, centroids1,precisionLevel);          
        // Get ending timepoint
        auto stop = high_resolution_clock::now();
          
        // Get duration. Substart timepoints to 
        // get durarion. To cast it to proper unit
        // use duration cast method
        auto duration = duration_cast<microseconds>(stop - start);
          
        std::cout << duration.count() << " us" << std::endl;
        std::cout << "P= " << precisionLevel << std::endl;

    	accLoss+=precisionLevel;
    	precLog << precisionLevel << std::endl;
      }
    	precLog.close();

	std::cout << "-------------------------------- " << std::endl;
	std::cout << "Relaxed convergence for K-Means = " << accLoss/times << std::endl;
	std::cout << "-------------------------------- " << std::endl;

//	kmeansApxLevel(precisionLevel);
#endif
return 0;
}

//appx version with configurable level of accuracy
void kmeans_series(point* space, point* centroids, int precisionLevel)
{
//int flips;
int relax;
int flipVector[20] = {51, 60, 82, 63, 74, 141, 155, 200, 270, 328, 449, 537, 691, 889, 1018, 1031, 1101, 1390, 3650, 33056};
int index = precisionLevel/5-1;

if (precisionLevel == 0){
    relax = 0;
}
else{
    if(index < 0)
    	index =0;
    if(index > 20)
    	index = 20;
    relax = flipVector[index];
}

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
// 	std::cout << "rx: " << rx << " ry: " << ry << " gx: " << gx << " gy: " << gy << "\n";
// 	std::cout << "bx: " << bx << " by: " << by << " rcount: " << rcount << " gcount: " << gcount<< " bcount: " << bcount << "\n";
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
	std::cout << "Flips = " << flips << std::endl;

}while(flips != relax);
}


void kmeans_Parallel(point* space, point* centroids, int precisionLevel)
{
//int flips;
int relax;
int flipVector[26] = {62, 59, 70, 71, 35, 69, 59, 27, 40, 95, 29, 79, 66, 88, 103, 112, 153, 163, 217, 333, 511, 846, 1741, 4188, 33174};
int index = precisionLevel/5-1;

if (precisionLevel == 0){
    relax = 0;
}
else{
    if(index < 0)
    	index =0;
    if(index > 26)
    	index = 26;
    relax = flipVector[index];
}
//  int relax = N*l/500; //N* (l/5) / 100 --l/5 % of N to be relaxed
		do{
    	flips = 0;
    	int rcount =0,gcount=0,bcount =0;
    	int rx,ry,gx,gy,bx,by;
    	rx = ry = bx = by = gx = gy = 0;
        pthread_create(&threads[0], &attr, kmeans_P0, (void *) &threads[0]);
        pthread_create(&threads[1], &attr, kmeans_P1, (void *) &threads[1]);
        pthread_create(&threads[2], &attr, kmeans_P2, (void *) &threads[2]);
        pthread_create(&threads[3], &attr, kmeans_P3, (void *) &threads[3]);
        
        for (int i=0; i<NTHREADS; i++) {
		 pthread_join(threads[i], NULL);
        }
    
        rx= rx0+rx1+rx2+rx3;
    	ry= ry0+ry1+ry2+ry3;
    	gx= gx0+gx1+gx2+gx3;
    	gy= gy0+gy1+gy2+gy3;
    	bx= bx0+bx1+bx2+bx3;
    	by= by0+by1+by2+by3;
    	rcount= rcount0+rcount1+rcount2+rcount3;
    	gcount= gcount0+gcount1+gcount2+gcount3;
    	bcount= bcount0+bcount1+bcount2+bcount3;
    // 	std::cout << "rx: " << rx << " ry: " << ry << " gx: " << gx << " gy: " << gy << "\n";
    // 	std::cout << "bx: " << bx << " by: " << by << " rcount: " << rcount << " gcount: " << gcount<< " bcount: " << bcount << "\n";

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
    	std::cout << "Flips = " << flips << std::endl;
    
    }while(flips != relax);
    //}while(flips > relax);
}


void* kmeans_P0(void *tid)
{
	for(int i=0; i<N/4; i++){
    	if(space[i].c == 'r'){
    	    rcount0++;
    	 	rx0 += space[i].x;
    	 	ry0 += space[i].y;
    // 	 	std::cout << "rx0: " << rx0 << " ry0: " << ry0 << " rcount0: " << rcount0 << "\n";
    	 }
    	 if(space[i].c == 'g'){
    	 	gcount0++;
    	 	gx0 += space[i].x;
    	 	gy0 += space[i].y;
    	 }
    	 if(space[i].c == 'b'){
    	 	bcount0++;
    	 	bx0 += space[i].x;
    	 	by0 += space[i].y;
    	 }
	  }
return NULL;
}

void* kmeans_P1(void *tid)
{
	for(int i=N/4; i<N/2; i++){
    	if(space[i].c == 'r'){
    	    rcount1++;
    	 	rx1 += space[i].x;
    	 	ry1 += space[i].y;
    	 }
    	 if(space[i].c == 'g'){
    	 	gcount1++;
    	 	gx1 += space[i].x;
    	 	gy1 += space[i].y;
    	 }
    	 if(space[i].c == 'b'){
    	 	bcount1++;
    	 	bx1 += space[i].x;
    	 	by1 += space[i].y;
    	 }
	  }
return NULL;
}

void* kmeans_P2(void *tid)
{
	for(int i=N/2; i<(N*(3/4)); i++){
    	if(space[i].c == 'r'){
    	    rcount2++;
    	 	rx2 += space[i].x;
    	 	ry2 += space[i].y;
    	 }
    	 if(space[i].c == 'g'){
    	 	gcount2++;
    	 	gx2 += space[i].x;
    	 	gy2 += space[i].y;
    	 }
    	 if(space[i].c == 'b'){
    	 	bcount2++;
    	 	bx2 += space[i].x;
    	 	by2 += space[i].y;
    	 }
	  }
return NULL;
}

void* kmeans_P3(void *tid)
{
	for(int i=(N*(3/4)); i<N; i++){
    	if(space[i].c == 'r'){
    	    rcount3++;
    	 	rx3 += space[i].x;
    	 	ry3 += space[i].y;
    	 }
    	 if(space[i].c == 'g'){
    	 	gcount3++;
    	 	gx3 += space[i].x;
    	 	gy3 += space[i].y;
    	 }
    	 if(space[i].c == 'b'){
    	 	bcount3++;
    	 	bx3 += space[i].x;
    	 	by3 += space[i].y;
    	 }
	  }
return NULL;
}

void FlushgVars (void)
{
    rx0 =0;
    ry0 =0;
    gx0 =0;
    gy0 =0;
    bx0 =0;
    by0 =0;
    rcount0 =0;
    gcount0 =0;
    bcount0 =0;
    rx1 =0;
    ry1 =0;
    gx1 =0;
    gy1 =0;
    bx1 =0;
    by1 =0;
    rcount1 =0;
    gcount1 =0;
    bcount1 =0;
	
    rx2 =0;
    ry2 =0;
    gx2 =0;
    gy2 =0;
    bx2 =0;
    by2 =0;
    rcount2 =0;
    gcount2 =0;
    bcount2 =0;

    rx3 =0;
    ry3 =0;
    gx3 =0;
    gy3 =0;
    bx3 =0;
    by3 =0;
    rcount3 =0;
    gcount3 =0;
    bcount3 =0;	
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

