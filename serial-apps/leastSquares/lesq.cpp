
#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <chrono>
#include <fstream>

#include "../../controller/ApplicationMonitor.h"


void printVec(std::vector<float> &v);
void dataGen(std::vector<float>&);
void lg(std::vector<float>&, std::vector<float>&);
void lgApx(std::vector<float>&, std::vector<float>&, int);

int main(int argc, char **argv)
{
	 std::ofstream precLog("precLog.txt",std::ios::out);
	 int times;
	 float accLoss=0;
	 unsigned long int N = 1000000;
	 std::vector<float> X, Y;
   X.resize(N);
   Y.resize(N);
   dataGen(X);
   dataGen(Y);
   times = std::atoi(argv[1]);
   data_t *data; //pointer to the shared memory to communicate with the monitor
   data = monitorAttach(argv[0], 20, 30, false);
	 
	 for(int i=0; i<times; i++){
    int precisionLevel = usePrecisionLevel(data);
		if(precisionLevel ==0)
			lg(X,Y);
		else{
		  if(precisionLevel < 5)
				precisionLevel = 5;
			lgApx(X,Y,precisionLevel);
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



void lg(std::vector<float> &X, std::vector<float> &Y)
{

 float sumx, sumxsq, sumy, sumxy,a0, a1, denom;
 sumx = 0;
 sumxsq = 0;
 sumy = 0;
 sumxy = 0;
 int N = X.size();
 for(int i = 0; i < N; i++){
    sumx +=  X[i];
    sumxsq += X[i]*X[i];
    sumy +=  Y[i];
    sumxy +=  X[i] * Y[i];
 }

 denom = (N * sumxsq) - (sumx*sumx);
 a0 = (sumy * sumxsq-sumx * sumxy) / denom;
 a1 = (N * sumxy-sumx * sumy) / denom;
}

void lgApx(std::vector<float> &X, std::vector<float> &Y, int loops)
{

 float sumx, sumxsq, sumy, sumxy,a0, a1, denom;
 sumx = 0;
 sumxsq = 0;
 sumy = 0;
 sumxy = 0;
 int N = X.size();
 for(int i = 0; i < N; i++) {
    if(i%100000==0)
			i+=1000*loops;
		
    sumx +=  X[i];
    sumxsq += X[i]*X[i];
    sumy +=  Y[i];
    sumxy +=  X[i] * Y[i];
 }

 denom = (N * sumxsq) - (sumx*sumx);
 a0 = (sumy * sumxsq-sumx * sumxy) / denom;
 a1 = (N * sumxy-sumx * sumy) / denom;

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
