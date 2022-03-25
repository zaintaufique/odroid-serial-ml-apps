#ifndef PARALLEL_APPX_POLICY_H_
#define PARALLEL_APPX_POLICY_H_

#include <fstream>
#include "Policy.h"
#include "ApplicationMonitor.h"
#include "getNode.h"
#include "utilization.h"


#define TDP_TH 4
#ifndef TDP
#define TDP 4
#endif
#define FMIN 200
#define FMAX_BIG 2000
#define FMAX_LITTLE 1400
#define PROFILE_PERIOD 3
#define MIG_BIAS 0.9 //threshold for bias of an application towards LITTLE
#define QA_POLICY
#define DVFS_APX_THRESHOLD 0.9 //threshold to check if dvfs+apx would work instead of TM
#define NUM_CORES 8
#define NEW_APP_WAIT_TIME 5

/*struct to store mapping information*/
typedef struct{
  //basic handle
  appl_t appl;

//application ids
  int app_id;
  pid_t pid;
	std::vector<int> core_id_vec;
  float quota;
  int num_threads;
  int apx_level;

//monitored parameters
float perf_factor;
  std::pair<int,float> dop_perf_little; //<num_of_threads, performance>
  std::pair<int,float> dop_perf_big;    //<num_of_threads, performance>

//run-time estimates
  float perf_estimate;
  float mig_perf_est;
  float big_perf_est;
  float perf_per_quota_est; //performance per quota allocated, estimation purpose
  
  std::pair<int,float> mig_perf; //<num_of_threads, performance>
  std::pair<int,float> big_perf;

//flags for control
  bool apx_flag;
  bool mig_apx_flag;
  bool firstMigDone;
  bool quota_flag;
  bool dvfs_flag;
  bool mig_flag;
  bool dop_flag;
}rtmInfo_prapx_t;

class ParallelAppxPolicy: public Policy{
  public:
    ParallelAppxPolicy();
    ~ParallelAppxPolicy();
    void run(int cycle);
    void printThroughput(monitor_t*);
    void printAccuracy(std::vector<rtmInfo_prapx_t>);
    void printPower();
    void printUtilization(std::vector<int>);
    void printFreq();

  private:
    monitor_t* appl_monitor_; // it keeps track of all aplications connected
    GetNode* sensors_; // using this pointer, we can get important values of the odroid, such as temperature, frequency and power
    Utilization* utilization_;
    std::ofstream powerLog, perfLog, utilLog, freqLog, mapLog, apxLog;
    bool dvfsDoneL;
    bool dvfsDoneB;
    int mapping_configuration_[8];
    std::vector<rtmInfo_prapx_t> running_apps_;
    int sleep_time_;
    bool decide_;
    bool profile_;
    bool idle_;
    bool refine_;
};

#endif