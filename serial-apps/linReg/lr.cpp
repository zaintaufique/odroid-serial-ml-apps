
#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <chrono>
#include <fstream>
#include "../../controller/ApplicationMonitor.h"

//full scale perf = 5.8
void printVec(std::vector<float> &v);
void dataGen(std::vector<float>&);
void lg(std::vector<float>&, std::vector<float>&, int);

int main(int argc, char **argv)
{
	 std::ofstream precLog("precLog.txt", std::ios::out);
	 int times;
	 float accLoss =0;
   times = std::atoi(argv[1]);
   data_t *data; //pointer to the shared memory to communicate with the monitor
   data = monitorAttach(argv[0], 8, 15, false);
	 int N = 500000;
   std::vector<float> X, Y;
   X.resize(N);
   Y.resize(N);
 	 dataGen(X);
   dataGen(Y);
   for(int i=0; i<times; i++){
	  	int precisionLevel = 2*usePrecisionLevel(data);
			accLoss+=precisionLevel;
      lg(X,Y,precisionLevel);
      monitorTick(data);
			precLog << precisionLevel << std::endl;
   }
  monitorDetach(data);
	precLog.close();
	std::cout << "------------------------------" << std::endl;
	std::cout << "Convergence relaxed for LinReg = " << accLoss/times << std::endl;
	std::cout << "------------------------------" << std::endl;

	
 return 0;
}




void lg(std::vector<float> &X, std::vector<float> &Y, int loops)
{
	double b0 = 0;
	double b1 = 0;
	double alpha = 0.01;
	int epochs = 4* X.size()*(100-loops)/100;
	for (int i = 0; i < epochs; i ++) {
    int idx = i % 5;
    double p = b0 + b1 * X[idx];
    double err = p - Y[idx];
    b0 = b0 - alpha * err;
    b1 = b1 - alpha * err * X[idx];
	}
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
