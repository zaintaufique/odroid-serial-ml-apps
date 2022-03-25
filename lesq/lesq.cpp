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

#include "../../controller/ApplicationMonitor.h"

//#define CONTROLLER_ATTACHED

//Pthread Initialization
#define NTHREADS 4
int arg[NTHREADS];
pthread_t threads[NTHREADS];
pthread_attr_t attr;

#define DATASIZE 1000000
std::vector<float> X, Y;

double gDenom =0;
double gA0 =0;
double gA1 =0;
int Data_points =0;

struct arg_t{
  int tid;
  int start,end;
  int sumx,sumxsq,sumy,sumxy;
};

void initData(void);
void printVec(std::vector<float> &v);
void dataGen(std::vector<float>&);
void lg(int N);
// void calc_global_sum (void);
void SeriesLesq(int N);
int calc_appx_iterations (int iterations, int loops);
// void FlushgVars (void);
void* ParallelLesq(void *arg);
void calcErorr(double Acc_a0, double Acc_a1, double Apx_a0, double Apx_a1);

int main(int argc, char **argv)
{
  int times = std::atoi(argv[1]);
  int iterations  =0;
  int precisionLevel =0;
  std::ofstream precLog("precLog.txt",std::ios::out);
  float accLoss=0;
  data_t *data; //pointer to the shared memory to communicate with the monitor
  data = monitorAttach(argv[0], 20, 30, false, true, NTHREADS);

  double Acc_a0, Acc_a1, Apx_a0, Apx_a1;
  for(int i=0; i<times; i++){
    initData();
    precisionLevel = usePrecisionLevel(data);
    if(precisionLevel ==0){    //Accurate Model
	     lg(Data_points);
	     //calc_global_sum();
	     Acc_a0 = gA0;
	     Acc_a1 = gA1;
	     //FlushgVars();
    }
    else{            //Approximate Model
        if(precisionLevel < 5){precisionLevel = 5;}
        iterations = calc_appx_iterations (Data_points, precisionLevel);
        lg(iterations);
        //calc_global_sum();
        Apx_a0 = gA0;
        Apx_a1 = gA1;
        //FlushgVars();
        //calcErorr(Acc_a0, Acc_a1, Apx_a0, Apx_a1);
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

void lg(int N)
{
  arg_t* args_list = new arg_t[NTHREADS];
	pthread_t* threads_list = new pthread_t[NTHREADS];
	int thread_no [NTHREADS];
	int BatchSize = N/NTHREADS;
	int i,j;

  int sumx = 0;
  int sumxsq = 0;
  int sumy = 0;
  int sumxy = 0;
  int gDenom = 0;
  gA0 = 0;
  gA1 = 0;
  // std::cout << "BatchSize: " << BatchSize << std::endl;
  int count =0;
  for (int i=0; i<NTHREADS; i++){
    args_list[i].sumx = 0;
    args_list[i].sumxsq = 0;
    args_list[i].sumy = 0;
    args_list[i].sumxy = 0;
	}

  for(i = 0, j = 0; i < N; i+=BatchSize, j+=1)
  {
    args_list[j].tid = j;
    args_list[j].start = i;
    args_list[j].end = std::min(i + BatchSize - 1, N - 1);
    pthread_create (&threads_list[j], NULL, ParallelLesq, (void*)&args_list[j]);
  }

  // Wait for threads to terminate
  for(int i=0; i<NTHREADS; i++){
    pthread_join(threads_list[i], NULL);
    sumx += args_list[i].sumx;
    sumxsq += args_list[i].sumxsq;
    sumy += args_list[i].sumy;
    sumxy += args_list[i].sumxy;
  }
  // std::cout << "sumx: " << sumx << " sumxsq: " << sumxsq << std::endl;
  // std::cout << "sumy: " << sumy << " sumxy: " << sumxy << std::endl;
  gDenom = (Data_points * sumxsq) - (sumx*sumx);
  gA0 = (sumy * sumxsq-sumx * sumxy) / gDenom;
  gA1 = (Data_points * sumxy-sumx * sumy) / gDenom;
}

void* ParallelLesq(void *arg)
{
   arg_t* info = (arg_t*) arg;
   int tid = info->tid;
   int start = info->start;
   int end = info->end;

   for(int i = start; i < end; i++){
    info->sumx +=  X[i];
    info->sumxsq += X[i]*X[i];
    info->sumy +=  Y[i];
    info->sumxy +=  X[i] * Y[i];
   }
   // pthread_exit (NULL);
   return 0;
}

void SeriesLesq(int N)
{
   double sumx, sumxsq, sumy, sumxy,a0, a1, denom;
   sumx = 0;
   sumxsq = 0;
   sumy = 0;
   sumxy = 0;
   for(int i = 0; i < N; i++){
    sumx +=  X[i];
    sumxsq += X[i]*X[i];
    sumy +=  Y[i];
    sumxy +=  X[i] * Y[i];
   }
   denom = (Data_points * sumxsq) - (sumx*sumx);
   a0 = (sumy * sumxsq-sumx * sumxy) / denom;
   a1 = (Data_points * sumxy-sumx * sumy) / denom;
//   std::cout << "sumx: " << sumx << " sumxsq: " << sumxsq << " sumy: " << sumy << " sumxy: " << sumxy << "\n";
//   std::cout << "Serial|| " << "denom: " << denom << " a0: " << a0 << " a1: " << a1 << "\n";
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

void calcErorr(double Acc_a0, double Acc_a1, double Apx_a0, double Apx_a1)
{
    double Y1, Y2, Res_Acc, Res_Apx;
    double Diff1 = 0;
    double Diff2 = 0;

    for (int i=0; i < Data_points; ++i)
    {
        Y1 = Acc_a0 + Acc_a1*X[i];
        Diff1 = Diff1 + (abs(Y1 - Y[i])*(abs(Y1 - Y[i])));

        Y2 = Apx_a0 + Apx_a1*X[i];
        Diff2 = Diff2 + (abs(Y2 - Y[i])*(abs(Y2 - Y[i])));
        // std::cout << "Diff1: " << Diff1 << " Diff2: " << Diff2 << std::endl;
    }
    Res_Acc = sqrt(Diff1/Data_points);
    Res_Apx = sqrt(Diff2/Data_points);
    //std::cout << "Acc: " << Res_Acc << " Apx: : " << Res_Apx << std::endl;
    std::cout << Res_Acc << "," << Res_Apx << std::endl;
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
