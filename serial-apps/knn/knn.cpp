#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <fstream>

#include "../../controller/ApplicationMonitor.h"

// full scale perf = 6.4

struct point{
	int x;
	int y;
	char val;
	char _class;

};

struct D{
	point p;
	float dist;
};

float distance (point&, point&);
std::vector<float> distanceVec(point, std::vector<point>);
void populateCluster(std::vector<point>&);
bool compareDist(const D &, const D &);
void printPoint(point);
int countClass(std::vector<D>, char);
void usage();
void knn (int, std::vector<point> &, std::vector<point> &);
void knnAppx (int, std::vector<point> &, std::vector<point> &);
void knnPrecision (int, std::vector<point> &, std::vector<point> &, int);




int main(int argc, char const *argv[])
{
	std::ofstream precLog("precLog.txt", std::ios::out);
	if(argc < 5)
		usage();
	int times;
	float accLoss=0;
  data_t *data; //pointer to the shared memory to communicate with the monitor
	times = atoi(argv[1]);
	data = monitorAttach(argv[0], 5, 12, false);

	int K, clusterSize, testSize;
	//K = std::atoi(argv[2]);
	K = 6;

	std::cout << "K = " << K << std::endl;

	std::vector<point> training_data;
	std::vector<point> test_data;
	//training_data.resize(std::atoi(argv[3]));
	training_data.resize(24000);
	std::cout << "Training data size = " << training_data.size() << std::endl;
	//test_data.resize(std::atoi(argv[4]));
	test_data.resize(20);
	std::cout << "Test data size = " << test_data.size() << std::endl;
	populateCluster(training_data);
	/*
	for(int i=0;i<training_data.size();i++){
		std::cout << "(" << training_data[i].x << "," << training_data[i].y << ")---" << training_data[i].val << std::endl;
	}
	*/
	populateCluster(test_data);

	for(int i=0; i<times; i++){
    int precisionLevel = usePrecisionLevel(data);
		//int precisionLevel = (i/10)*5;
		accLoss+= precisionLevel;
		if(precisionLevel==0){
					 knn(K, training_data, test_data);
		}
		else{
					 knnPrecision(K, training_data, test_data, precisionLevel);
		}
		
		precLog << precisionLevel << std::endl;    
    monitorTick(data);
  }

  monitorDetach(data);
	precLog.close();
  std::cout << "-----------------------------" << std::endl;
	std::cout << "KNN accuracy loss overall = " << accLoss/times << std::endl;
  std::cout << "-----------------------------" << std::endl;




	return 0;
}

void knn (int K, std::vector<point> &training_data, std::vector<point> &test_data)
{
	for(int i=0; i< test_data.size(); i++){
		std::vector<D> d;
		d.resize(training_data.size());
		for(int j=0; j< training_data.size(); j++){
			d[j].p = training_data[j];
			d[j].dist = distance(test_data[i], training_data[j]);
			//std::cout << "Point ";
			//printPoint(d[j].p);
			//std::cout << "Distance: " << d[j].dist << std::endl;
		}
		std::sort (d.begin(), d.end(), compareDist);
		/*
		for(int k=0; k< d.size(); k++){
			//std::cout << "(" << d[k].p.x << "," << d[k].p.y << ")---" << d[k].dist << std::endl;
			printPoint(d[i].p);
		}
		*/
		int count =0, index=0;

		for(int i=0; i<K ; i++){
			int tmpcount = countClass(d, d[i].p._class);
			if(tmpcount > count){
				count = tmpcount;
				index = i;
			}


		}
		test_data[i]._class = d[index].p._class;
	/*	std::cout << "CLASSIFIED POINT (" ;
		printPoint(test_data[i]);
		std::cout << "AS ---" << test_data[i]._class << std::endl;
*/

	}

}




void knnPrecision (int K, std::vector<point> &training_data, std::vector<point> &test_data, int prec)
{
	int skip = 100/prec;
	for(int i=0; i< test_data.size(); i++){
		if(i%skip==0)
			continue;
		std::vector<D> d;
		d.resize(training_data.size());
		for(int j=0; j< training_data.size(); j++){
			if(j%2000==0)
				j+= 20*prec;
			d[j].p = training_data[j];
			d[j].dist = distance(test_data[i], training_data[j]);
			//std::cout << "Point ";
			//printPoint(d[j].p);
			//std::cout << "Distance: " << d[j].dist << std::endl;
		}
		std::sort (d.begin(), d.end(), compareDist);
		/*
		for(int k=0; k< d.size(); k++){
			//std::cout << "(" << d[k].p.x << "," << d[k].p.y << ")---" << d[k].dist << std::endl;
			printPoint(d[i].p);
		}
		*/
		int count =0, index=0;

		for(int i=0; i<K ; i++){
			int tmpcount = countClass(d, d[i].p._class);
			if(tmpcount > count){
				count = tmpcount;
				index = i;
			}


		}
		test_data[i]._class = d[index].p._class;
	/*	std::cout << "CLASSIFIED POINT (" ;
		printPoint(test_data[i]);
		std::cout << "AS ---" << test_data[i]._class << std::endl;
*/

	}

}

void knnAppx (int K, std::vector<point> &training_data, std::vector<point> &test_data)
{
	K = K/2;
	for(int i=0; i< test_data.size(); i++){
		std::vector<D> d;
		d.resize(training_data.size());
		for(int j=0; j< training_data.size(); j++){
			d[j].p = training_data[j];
			d[j].dist = distance(test_data[i], training_data[j]);
			//std::cout << "Point ";
			//printPoint(d[j].p);
			//std::cout << "Distance: " << d[j].dist << std::endl;
		}
		std::sort (d.begin(), d.end(), compareDist);
		/*
		for(int k=0; k< d.size(); k++){
			//std::cout << "(" << d[k].p.x << "," << d[k].p.y << ")---" << d[k].dist << std::endl;
			printPoint(d[i].p);
		}
		*/
		int count =0, index=0;

		for(int i=0; i<K ; i++){
			int tmpcount = countClass(d, d[i].p._class);
			if(tmpcount > count){
				count = tmpcount;
				index = i;
			}


		}
		test_data[i]._class = d[index].p._class;
	/*	std::cout << "CLASSIFIED POINT (" ;
		printPoint(test_data[i]);
		std::cout << "AS ---" << test_data[i]._class << std::endl;
*/

	}

}


float distance (point &p1, point &p2)
{
	//printPoint(p1);
	//printPoint(p2);
		return sqrt((((p1).x)-p2.x)*(p1.x-p2.x))+((p1.y-p2.y)*(p1.y-p2.y));

}

std::vector<float> distanceVec (point p, std::vector<point> C)
{
	std::vector<float> dist;
	dist.resize(C.size());
	for(int i=0; i< C.size(); i++)
		dist.push_back(sqrt(((p.x-C[i].x)*(p.x-C[i].x))+((p.y-C[i].y)*(p.y-C[i].y))));
	return dist;

}


void populateCluster(std::vector<point> &cluster)
{

  auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::mt19937 _rand(seed);
	for(int i=0; i< cluster.size(); i++){
		cluster[i].x = _rand()%150;
		cluster[i].y = _rand()%100;
		cluster[i].val = char(65+_rand()%5);
		cluster[i]._class = 'a';

	}


}

bool compareDist(const D &d1, const D &d2)
{
	return d1.dist < d2.dist;
}

void printPoint(point p)
{
	std::cout << "(" << p.x << "," << p.y << ")---" << p.val << "---"  << p._class << std::endl;
}

int countClass(std::vector<D> d, char c)
{
	int cnt=0;
	for(int i=0; i< d.size(); i++){
		if(d[i].p._class == c)
			cnt++;
	}
	return cnt;
}

void usage()
{
	std::cout << "Usage:" << std::endl << "./exe times K train test" << std::endl;
	std::cout << "times: no.of.inner loops\t K: no.of.means \t train: training data size \t test: test data size" << std::endl;
}
