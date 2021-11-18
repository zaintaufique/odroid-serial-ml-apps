#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <chrono>
#include <fstream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


//#define CONTROLLER_ATTACHED

#ifdef CONTROLLER_ATTACHED
#include "../../controller/ApplicationMonitor.h"
#else

//Pthread Initialization
#define NTHREADS 4
int arg[NTHREADS];
pthread_t threads[NTHREADS];
pthread_attr_t attr;

double Tsumx =0;
double sumx0 =0;
double sumx1 =0;
double sumx2 =0;
double sumx3 =0;

double Tsumxsq =0;
double sumxsq0 =0;
double sumxsq1 =0;
double sumxsq2 =0;
double sumxsq3 =0;

double Tsumy = 0;
double sumy0 =0;
double sumy1 =0;
double sumy2 =0;
double sumy3 =0;

double Tsumxy =0;
double sumxy0 =0;
double sumxy1 =0;
double sumxy2 =0;
double sumxy3 =0;

#define DATASIZE 1000000
std::vector<float> X, Y;
// int loops=0;
int iterations  =0;

double gDenom =0;
double gA0 =0;
double gA1 =0;
#endif

int Data_points =0;

void initData(void);
void printVec(std::vector<float> &v);
void dataGen(std::vector<float>&);
void lg(void);
void calc_global_sum (void);
void SeriesLesq(void);
int calc_appx_iterations (int iterations, int loops);
void FlushgVars (void);
void* ParallelLesq0(void *tid);
void* ParallelLesq1(void *tid);
void* ParallelLesq2(void *tid);
void* ParallelLesq3(void *tid);

int main(int argc, char **argv)
{
  int times = 1;//std::atoi(argv[1]);
  int precisionLevel;

  #ifdef CONTROLLER_ATTACHED
    std::ofstream precLog("precLog.txt",std::ios::out);
    float accLoss=0;
    initData();
    data_t *data; //pointer to the shared memory to communicate with the monitor
    data = monitorAttach(argv[0], 20, 30, false);
    for(int i=0; i<times; i++){
       precisionLevel = usePrecisionLevel(data);
       if(precisionLevel ==0){    //Accurate Model
         iterations = Data_points;
         SeriesLesq();
       }
       else{            //Approximate Model
         if(precisionLevel < 5)
           precisionLevel = 5;
             iterations = calc_appx_iterations (Data_points, precisionLevel);
             SeriesLesq();
      }
      precLog << precisionLevel << std::endl;
      accLoss+=precisionLevel;
      monitorTick(data);
     }
     monitorDetach(data);
     precLog.close();

     std::cout << " ------------------------------- " << std::endl;
     std::cout << "Accuracy loss LeSq = "<< accLoss/times << std::endl;
     std::cout << " ------------------------------- " << std::endl;
  #else
     for(int i=0; i<times; i++){
        initData();
        //Accurate Model
        std::cout << "Accurate \n";
        iterations = Data_points;
        lg();
        calc_global_sum();
            FlushgVars();
      //Approximate Model
      precisionLevel =5;
      iterations = calc_appx_iterations (Data_points, precisionLevel);
      std::cout << "Approximate \n";
      lg();
            calc_global_sum();
        FlushgVars();
      //********************//
      //Test results
//      iterations = Data_points;
//      SeriesLesq();
//      iterations = calc_appx_iterations (Data_points, precisionLevel);
//      SeriesLesq();
      //*******************//
    }
    /* Clean up and exit */
    pthread_attr_destroy(&attr);
//    pthread_mutex_destroy(&res_mutex);
    pthread_exit (NULL);
  #endif
   return 0;
}

void initData(void)
{
  X.resize(DATASIZE);
  Y.resize(DATASIZE);
  Data_points = X.size();
  dataGen(X);
  dataGen(Y);
}

void lg(void)
{
  pthread_create(&threads[0], &attr, ParallelLesq0, (void *) &threads[0]);
  pthread_create(&threads[1], &attr, ParallelLesq1, (void *) &threads[1]);
  pthread_create(&threads[2], &attr, ParallelLesq2, (void *) &threads[2]);
  pthread_create(&threads[3], &attr, ParallelLesq3, (void *) &threads[3]);

    for (int i=0; i<NTHREADS; i++) {
     pthread_join(threads[i], NULL);
    }
}

void SeriesLesq(void)
{
   double sumx, sumxsq, sumy, sumxy,a0, a1, denom;

   sumx = 0;
   sumxsq = 0;
   sumy = 0;
   sumxy = 0;
   for(int i = 0; i < iterations; i++){
    sumx +=  X[i];
      sumxsq += X[i]*X[i];
      sumy +=  Y[i];
      sumxy +=  X[i] * Y[i];

   }
   denom = (Data_points * sumxsq) - (sumx*sumx);
   a0 = (sumy * sumxsq-sumx * sumxy) / denom;
   a1 = (Data_points * sumxy-sumx * sumy) / denom;
   std::cout << "sumx: " << sumx << " sumxsq: " << sumxsq << " sumy: " << sumy << " sumxy: " << sumxy << "\n";
   std::cout << "Serial|| " << "denom: " << denom << " a0: " << a0 << " a1: " << a1 << "\n";
}

void* ParallelLesq0(void *tid)
{
   for(int i = 0; i < iterations/4; i++){
    sumx0 +=  X[i];
      sumxsq0 += X[i]*X[i];
      sumy0 +=  Y[i];
      sumxy0 +=  X[i] * Y[i];
   }
   pthread_exit (NULL);
}

void* ParallelLesq1(void *tid)
{
   for(int i = iterations/4; i < iterations/2; i++){
    sumx1 +=  X[i];
      sumxsq1 += X[i]*X[i];
      sumy1 +=  Y[i];
      sumxy1 +=  X[i] * Y[i];
   }
   pthread_exit (NULL);
}

void* ParallelLesq2(void *tid)
{
   for(int i = iterations/2; i < (3*iterations/4); i++){
    sumx2 +=  X[i];
      sumxsq2 += X[i]*X[i];
      sumy2 +=  Y[i];
      sumxy2 +=  X[i] * Y[i];
   }
   pthread_exit (NULL);
}

void* ParallelLesq3(void *tid)
{
   for(int i = (3*iterations/4); i < iterations; i++){
    sumx3 +=  X[i];
      sumxsq3 += X[i]*X[i];
      sumy3 +=  Y[i];
      sumxy3 +=  X[i] * Y[i];
   }
   pthread_exit (NULL);
}

void dataGen(std::vector<float> &v)
{
  auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::mt19937 randEngine(seed);
  for(int i=0; i< v.size(); i++){
    v[i]= (randEngine()%1000);
  }
}

void printVec(std::vector<float> &v )
{
  for(int i=0; i<v.size();i++)
    std::cout << v[i] << std::endl;
}

void calc_global_sum (void)
{
    double gA0, gA1, gDenom;
    Tsumx   = sumx0 + sumx1 + sumx2 + sumx3;
  Tsumxsq = sumxsq0 + sumxsq1 + sumxsq2 + sumxsq3;
  Tsumy   = sumy0 + sumy1 + sumy2 + sumy3;
  Tsumxy  = sumxy0 + sumxy1 + sumxy2 + sumxy3;
  gDenom = (Data_points * Tsumxsq) - (Tsumx*Tsumx);
  gA0 = (Tsumy * Tsumxsq-Tsumx * Tsumxy) / gDenom;
  gA1 = (Data_points * Tsumxy-Tsumx * Tsumy) / gDenom;
  std::cout << "Tsumx: " << Tsumx << " Tsumxsq: " << Tsumxsq << " Tsumy: " << Tsumy << " Tsumxy: " << Tsumxy << "\n";
  std::cout << "denom: " << gDenom << " a0: " << gA0 << " a1: " << gA1 << "\n";
}

void FlushgVars (void)
{
    Tsumx =0;
    sumx0 =0;
    sumx1 =0;
    sumx2 =0;
    sumx3 =0;

    Tsumxsq =0;
    sumxsq0 =0;
    sumxsq1 =0;
    sumxsq2 =0;
    sumxsq3 =0;

    Tsumy = 0;
    sumy0 =0;
    sumy1 =0;
    sumy2 =0;
    sumy3 =0;

    Tsumxy =0;
    sumxy0 =0;
    sumxy1 =0;
    sumxy2 =0;
    sumxy3 =0;

    iterations  =0;

    gDenom =0;
    gA0 =0;
    gA1 =0;
}

int calc_appx_iterations (int iterations, int loops)
{
    int appx_iteration=0;
    for(int i = 0; i < iterations; i++){
        if (i%100000==0)
      {
          i+=1000*loops;
      }
      ++appx_iteration;
    }
    return appx_iteration;
}
