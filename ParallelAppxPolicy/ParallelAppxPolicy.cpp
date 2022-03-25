//dop_perf_little//testing on windows
#include "ParallelAppxPolicy.h"
#include <iostream>
#include "CGroupUtils.h"
#include <cmath>
#include <algorithm>
#include <utility>

ParallelAppxPolicy::ParallelAppxPolicy() {
    this->appl_monitor_ = monitorInit();
    //this->sensors_ = new GetNode(GetNode::NO_POWER_sensors_);
    this->sensors_ = new GetNode();
    this->utilization_ = new Utilization(NUM_CORES);
    this->powerLog.open("../controller/Traces/power.log", std::ios::out);
    this->perfLog.open("../controller/Traces/perf.log", std::ios::out);
    this->utilLog.open("../controller/Traces/util.log", std::ios::out);
    this->freqLog.open("../controller/Traces/freq.log", std::ios::out);
    //  this->apxLog.open("../controller/Traces/apx.log", std::ios::out);
      //reset big and little cluster mapping status
    for (int i = 0; i < NUM_CORES; i++) {
        this->mapping_configuration_[i] = 0;
    }
    //set both the clusters to minimum frequency
    this->sensors_->setCPUFreq(0, FMIN);
    this->sensors_->setCPUFreq(4, FMIN);
    this->sleep_time_ = 0;
    this->dvfsDoneL = false;
    this->dvfsDoneB = false;
    this->profile_ = false;
    this->decide_ = false;
    this->refine_ = false;
}

ParallelAppxPolicy::~ParallelAppxPolicy() {
    killAttachedApplications(this->appl_monitor_);
    monitorDestroy(this->appl_monitor_);
    delete sensors_;
    delete utilization_;
    this->powerLog.close();
    this->perfLog.close();
    this->utilLog.close();
    this->freqLog.close();
    //this->apxLog.close();
    this->sensors_->setCPUFreq(0, FMIN);
    this->sensors_->setCPUFreq(4, FMIN);
}

void ParallelAppxPolicy::run(int cycle) {
    std::vector<pid_t> newAppls = updateAttachedApplications(this->appl_monitor_);
    std::vector<int> u = utilization_->getCPUUtilization();
    this->sensors_->updateSensing(u); // updating sensors_ before printing of them values
    rtmInfo_prapx_t tempApp;
    std::vector<int> uForEstimation;
    uForEstimation.resize(NUM_CORES);
    for (int y = 0; y < NUM_CORES; y++) {
        uForEstimation[y] = 0;
    }
    bool newAppFlag = false;
    bool appLeftFlag = false;
    /******************HANDLING NEW APP****************/
    if (newAppls.size() > 0) {
        std::vector<int> freeCores;
        bool coreFolded = false;
        for (int i = 0; i < newAppls.size(); ++i) {
            for (int j = 0; j < this->appl_monitor_->nAttached; j++) {
                if (this->appl_monitor_->appls[j].pid == newAppls[i]) {
                    for (int k = 0; k < 4; k++) {	//Check mapping conf on little cores
                        if (this->mapping_configuration_[k] == 0) {
                            freeCores.push_back(k); //??
                            std::cout << "Core On Litte: " << k << std::endl;
                        }
                    }
                    if (freeCores.size() == 0) { //no LITTLE cores found, then find big cores
                        for (int k = 4; k < 8; k++) {
                            if (this->mapping_configuration_[k] == 0) {
                                freeCores.push_back(k);
                                std::cout << "Core On Big: " << k << std::endl;
                            }
                        }
                    }
                    if (freeCores.size() == 0) {  //No Core Found
                        std::cout << "Find a Parallel app on big" << std::endl;
                        for (int i = 0; i < this->running_apps_.size(); i++) { //iterate through the apps
                            if (this->running_apps_[i].core_id_vec.front() > 3) { //Go to the apps on the big core
                                std::cout << "App found!!!" << std::endl;
                                if (this->running_apps_[i].appl.is_parallel) { //Check if the app is parallel
                                    std::vector<int> foldCores;
                                    std::cout << "Fold the Parallel App by one cores" << std::endl;
                                    for (int k = this->running_apps_[i].core_id_vec.front(); k < (this->running_apps_[i].core_id_vec.back()); k++) { //reduce one core
                                        foldCores.push_back(k);
                                    }
                                    if (foldCores.size()){
                                        //Set freeCores equal to the unfolded core number
                                        freeCores.push_back(running_apps_[i].core_id_vec.back());
                                        //Map the existing app on the folded number of cores
                                        CGUtils::UpdateCpuSetParallel(this->running_apps_[i].pid, foldCores); //MAP ON CORE //MAP on big
                                        //reset previuos mapping
                                        for (int q = 0; q < this->running_apps_[i].core_id_vec.size(); q++) {
                                            this->mapping_configuration_[this->running_apps_[i].core_id_vec[q]] = 0;
                                        }
                                        //update current mapping
                                        for (int q = 0; q < foldCores.size(); q++) {
                                            this->mapping_configuration_[foldCores[q]] = 1;
                                        }
                                        //update the app.
                                        this->running_apps_[i].core_id_vec = foldCores;
                                        coreFolded = true;
                                    }
                                    else{
                                        std::cout << "App Folding Failed" << std::endl;
                                    }
                                }
                            }
                        }
                        if (coreFolded == false) { std::cout << "No more cores found!" << std::endl; }
                    }
                    if (coreFolded == true || (freeCores.size() != 0)) { //CORE FOUND, MAP THE APP
                        if (this->appl_monitor_->appls[j].is_parallel) {
                            std::cout << "Map on Parallel " << std::endl;
                            CGUtils::UpdateCpuSetParallel(newAppls[i], freeCores); //MAP ON CORE
                        }
                        else {
                            std::cout << "Map on Serial " << std::endl;
                            freeCores.erase(freeCores.begin() + 1, freeCores.end());    //We need only one core for Serial App
                            CGUtils::UpdateCpuSet(newAppls[i], freeCores); //MAP ON CORE
                        }
                        for (int k = 0; k < freeCores.size(); k++) {    //UPDATE THE MAPPING Config
                            tempApp.core_id_vec.push_back(freeCores[k]);
                            this->mapping_configuration_[freeCores[k]] = 1;
                        }
                        //UPDATE THE rtm rtmInfo
                        tempApp.appl = this->appl_monitor_->appls[j];
                        tempApp.pid = this->appl_monitor_->appls[j].pid;
                        tempApp.num_threads = freeCores.size();
                        if (freeCores.front() < 4) {
                            tempApp.mig_perf.first = freeCores.size();
                            tempApp.mig_perf.second = 0;
                            tempApp.big_perf.first = 0;
                            tempApp.big_perf.second = -1;//this value <1 indicates no past info on big performance
                            tempApp.mig_perf_est = tempApp.mig_perf.second;
                        }
                        else {
                            tempApp.big_perf.first = freeCores.size();
                            tempApp.big_perf.second = 0;
                            tempApp.mig_perf.first = 0;
                            tempApp.mig_perf.second = -1;//this value <1 indicates no past info on LITTLE performance
                            tempApp.mig_perf_est = tempApp.mig_perf.second;
                        }
                        tempApp.num_threads = freeCores.size();
                        tempApp.apx_level = 0;
                        tempApp.quota = freeCores.size(); //consider that each core has a quota of 1, so an application may have total quota > 1
                        tempApp.apx_flag = false;
                        tempApp.mig_apx_flag = false;
                        tempApp.quota_flag = false;
                        tempApp.dvfs_flag = false;
                        tempApp.mig_flag = false;
                        tempApp.dop_flag = false;
                        tempApp.firstMigDone = false;

                        this->running_apps_.push_back(tempApp);
                        for (int k = 0; k < freeCores.size(); k++) {
                            uForEstimation[freeCores[k]] = 100 * tempApp.quota / freeCores.size(); //total quota divided by num of cores gives per core quota
                        }
                        newAppFlag = true;

                        this->profile_ = true;
                        this->decide_ = false;
                        this->refine_ = false;
                    }
                }
            }
        }
        //print of the new applications
        std::cout << "New applications: ";
        for (int k = 0; k < newAppls.size(); k++) {
            std::cout << newAppls[k] << " ";
        }
        std::cout << std::endl;
    }
    else {
        for (int i = 0; i < this->running_apps_.size(); i++) {
            bool foundApp = false;
            //FIND NEW APP
            for (int j = 0; j < this->appl_monitor_->nAttached && !foundApp; j++) {
                if (this->running_apps_[i].pid == this->appl_monitor_->appls[j].pid) {
                    foundApp = true;
                    this->running_apps_[i].app_id = j;
                }
            }
            // if some application must have left..then...
            if (!foundApp) {
                appLeftFlag = true;
                std::cout << "App " << running_apps_[i].app_id << " left" << std::endl;
                for (int k = 0; k < this->running_apps_[i].core_id_vec.size(); k++) {
                    this->mapping_configuration_[this->running_apps_[i].core_id_vec[k]] = 0;
                    std::cout << "setting core " << this->running_apps_[i].core_id_vec[k] << " map status to "
                        << this->mapping_configuration_[this->running_apps_[i].core_id_vec[k]] << std::endl;
                }
                this->running_apps_.erase(this->running_apps_.begin() + i);
                i--;
                this->profile_ = false;
                this->decide_ = true;
                this->refine_ = false;
            }
        }
        /******************HANDLING STATE MACHINE****************/
        if (this->appl_monitor_->nAttached > 0 && !appLeftFlag) {
            data_t dataTemp;
            float perf1, perf2;
            //update performance of all apps
            //update current utilization
            //reset all knob flags
            for (int i = 0; i < this->running_apps_.size(); i++) {
                dataTemp = monitorRead(appl_monitor_->appls[running_apps_[i].app_id].segmentId);
                perf1 = getCurrThroughput(&dataTemp);
                perf2 = (this->appl_monitor_->appls[this->running_apps_[i].app_id].tpMinObj + this->appl_monitor_->appls[this->running_apps_[i].app_id].tpMaxObj) / 2;
                running_apps_[i].perf_factor = perf1 / perf2;
                running_apps_[i].perf_estimate = running_apps_[i].perf_factor;
                for (int q = 0; q < running_apps_[i].core_id_vec.size(); q++) {
                    uForEstimation[running_apps_[i].core_id_vec[q]] = running_apps_[i].quota * 100 / running_apps_[i].core_id_vec.size();
                }
                running_apps_[i].apx_flag = false;
                running_apps_[i].quota_flag = false;
                running_apps_[i].dvfs_flag = false;
                running_apps_[i].mig_flag = false;
                running_apps_[i].dop_flag = false;
            }
            //============================================================
            //===================PROFILE PHASE============================
            //============================================================
            if (this->profile_ && !this->decide_ && !this->refine_) {
                std::cout << "=========PROFILE==========" << std::endl;
                if (sleep_time_ == PROFILE_PERIOD) {
                    //collect performance information on LITTLE for future reference
                    for (int i = 0; i < this->running_apps_.size(); i++) {
                        if (this->running_apps_[i].mig_perf.second == 0 && this->running_apps_[i].big_perf.first <= 0) {
                            this->running_apps_[i].mig_perf.second = this->running_apps_[i].perf_factor;
                            this->running_apps_[i].mig_perf_est = this->running_apps_[i].mig_perf.second;
                            //std::cout << "MigPerfEst: " << this->running_apps_[i].mig_perf_est << std::endl;
                            //std::cout << "mig_perf.second: " << this->running_apps_[i].mig_perf.second << std::endl;
                        }
                        else if (this->running_apps_[i].big_perf.second == 0 && this->running_apps_[i].mig_perf.first <= 0) {
                            this->running_apps_[i].big_perf.second = this->running_apps_[i].perf_factor;
                            this->running_apps_[i].big_perf_est = this->running_apps_[i].big_perf.second;
                            //std::cout << "big_perf_est: " << this->running_apps_[i].big_perf_est << std::endl;
                            //std::cout << "big_perf.second : " << this->running_apps_[i].big_perf.second << std::endl;
                        }
                    }
                    this->sleep_time_ = 0;
                    this->profile_ = false;
                    this->decide_ = true;
                    this->refine_ = false;
                }
                else {
                    //increment the cycles to continue PROFILE phase
                    this->sleep_time_++;
                }
            }
            else if (!this->profile_ && this->decide_ && !this->refine_) {
                std::cout << "=========DECIDE==========" << std::endl;
                float powerNow = this->sensors_->getBigW() + this->sensors_->getLittleW();
                float powerFactor = TDP / powerNow;
                if (powerNow >= TDP) {      //represents power violation
                    std::cout << "====TDP === Violation ====" << std::endl;
                    bool dvfsDone = false;
                    bool migDone = false;
                    bool dopDone = false;
                    bool doMigration = false;
                    //Option 1: DOP
                    for (int i = 0; i < this->running_apps_.size(); i++) {
                        if (this->running_apps_[i].core_id_vec.front() > 3 && this->running_apps_[i].appl.is_parallel) {
                            std::vector<int> unfoldCores;
                            std::cout << "Option:1 Dop" << std::endl;
                            for (int q = 4; q < 8; q++) {
                                if (this->mapping_configuration_[q] == 0) {
                                    unfoldCores.push_back(q);
                                }
                            }
                            if (unfoldCores.size()) { //Free Cores Found
                                std::cout << "Unfold the Parallel App" << std::endl;
                                //Set freeCores equal to the unfolded core number
                                unfoldCores.push_back(running_apps_[i].core_id_vec.back());
                                //Map the existing app on the folded number of cores
                                CGUtils::UpdateCpuSetParallel(this->running_apps_[0].pid, unfoldCores); //MAP ON CORE //MAP on big
                                //reset previuos mapping
                                for (int q = 0; q < this->running_apps_[i].core_id_vec.size(); q++) {
                                    this->mapping_configuration_[this->running_apps_[i].core_id_vec[q]] = 0;
                                }
                                //update current mapping
                                for (int q = 0; q < unfoldCores.size(); q++) {
                                    this->mapping_configuration_[unfoldCores[q]] = 1;
                                }
                                //update the app.
                                this->running_apps_[i].core_id_vec = unfoldCores;
                                this->running_apps_[i].dop_flag = true;
                                //since changed DoP and thus mapping, come back to decide_ again..
                                this->profile_ = false;
                                this->decide_ = true;
                                this->refine_ = false;
                                dopDone = true;
                            }
                            else {
                                std::cout << "No Free Cores found. Option:2 Migration" << std::endl;
                                doMigration = true;
                            }
                        }
                        else {
                            doMigration = true;
                        }
                    }
                    //Option 2: Migration
                    if (doMigration) {
                        std::cout << "==== Test Migration ====" << std::endl;
                        //find all free LITTLE cores for Task migration as a first option.
                        std::vector<int> migCores;
                        for (int q = 0; q < 4; q++) {
                            if (this->mapping_configuration_[q] == 0) {
                                migCores.push_back(q);
                            }
                        }
                        if (migCores.size() == 0) { std::cout << "No free cores on LITTLE" << std::endl; }
                        //if there are any free cores on LITTLE for migration..
                        if (migCores.size()) {
                            //get apps with highest bias, also linearly scaled up with DoP
                            for (int i = 0; i < this->running_apps_.size(); i++) {
                                //for apps running on big with past info on their LITTLE performance..
                                if (this->running_apps_[i].core_id_vec.front() > 3 && this->running_apps_[i].mig_perf.first > 0) {
                                    //if performance on LITTLE is less than BIAS
                                    if (this->running_apps_[i].mig_perf.second < MIG_BIAS) {
                                        if (this->running_apps_[i].core_id_vec.size() < migCores.size()) {
                                            this->running_apps_[i].mig_perf_est = this->running_apps_[i].mig_perf.second * migCores.size() / this->running_apps_[i].mig_perf.first;
                                        }
                                    }
                                }
                            }
                        }
                        //find app with highest bias
                        std::sort(this->running_apps_.begin(), this->running_apps_.end(),
                            [](const rtmInfo_prapx_t& a, const rtmInfo_prapx_t& b)
                            {return a.mig_perf_est > b.mig_perf_est; }
                        );
                        //Checking Bias to see if migration can help
                        if (this->running_apps_[0].mig_perf_est >= MIG_BIAS && (migCores.size()) && (this->running_apps_[0].core_id_vec.front() > 3)) {
                            std::cout << "One app found for migration" << std::endl;
                            //if there is at least 1 app that is a candidate for task migration..
                              //do task migration for the best fit app.
                            if (this->running_apps_[0].appl.is_parallel) {
                                CGUtils::UpdateCpuSetParallel(this->running_apps_[0].pid, migCores);
                            }
                            else {
                                migCores.erase(migCores.begin() + 1, migCores.end());
                                CGUtils::UpdateCpuSet(this->running_apps_[0].pid, migCores);
                            }
                            //reset previuos mapping
                            for (int q = 0; q < this->running_apps_[0].core_id_vec.size(); q++) {
                                this->mapping_configuration_[this->running_apps_[0].core_id_vec[q]] = 0;
                            }
                            //update current mapping
                            for (int q = 0; q < migCores.size(); q++) {
                                this->mapping_configuration_[migCores[q]] = 1;
                            }
                            //update the app.
                            this->running_apps_[0].core_id_vec = migCores;

                            //if task migration leads to perf loss, do appx
                            //TODO- Add levels of appx from look up tables here..
                            if (this->running_apps_[0].mig_perf_est < 1) {
                                int level = 100 * (1 - running_apps_[0].mig_perf_est);
                                if (level < 0) level = 0; {
                                    level = ((level + 5) / 10) * 10;
                                }
                                data_t* applSHM = monitorPtrRead(this->appl_monitor_->appls[this->running_apps_[0].app_id].segmentId);
                                if (this->running_apps_[0].apx_level + level < 100 && this->running_apps_[0].apx_level + level >= 0) {
                                    running_apps_[0].apx_level += level;
                                }
                                setPrecisionLevel(applSHM, running_apps_[0].apx_level);
                                this->running_apps_[0].mig_apx_flag = true;
                            }
                            migDone = true;
                            this->profile_ = true;
                            this->decide_ = false;
                            this->refine_ = false;
                        }
                    }
                    //Option 3: DVFS
                    if (migDone == false) {
                        std::cout << "==== Test DVFS ====" << std::endl;
                        int f1 = this->sensors_->getCPUCurFreq(7);
                        int f2 = f1 * powerFactor;
                        f2 = ((f2 + 50) / 100) * 100;
                        if (f2 < FMIN) { f2 = FMIN; }
                        if (f2 > FMAX_BIG) { f2 = FMAX_BIG; }
                        if (f1 != f2) {
                            this->sensors_->setCPUFreq(7, f2);
                            dvfsDone = true;
                            //estimate performance loss, if any, with DVFS
                            for (int i = 0; i < this->running_apps_.size(); i++) {
                                this->running_apps_[i].perf_estimate = this->running_apps_[i].perf_factor * f2 / f1;
                            }
                            //sort apps as per their performance loss
                            std::sort(this->running_apps_.begin(), this->running_apps_.end(),
                                [](const rtmInfo_prapx_t& a, const rtmInfo_prapx_t& b)
                                {return a.perf_estimate < b.perf_estimate; }
                            );

                            for (int i = 0; i < this->running_apps_.size(); i++) {
                                if (this->running_apps_[i].perf_estimate < 1) {
                                    if (!this->running_apps_[i].appl.is_parallel) {
                                        if (this->running_apps_[i].quota < this->running_apps_[i].core_id_vec.size()) {
                                            std::cout << "==== Test Quota ====" << std::endl;
                                            float quotaReq = this->running_apps_[i].quota / this->running_apps_[i].perf_factor;
                                            if (quotaReq > running_apps_[i].core_id_vec.size()) {
                                                quotaReq = running_apps_[i].core_id_vec.size();
                                            }
                                            std::vector<int> uTemp = uForEstimation;
                                            //keep a copy of util; this will be the uForEstimation back again, in case power factor >1 with new quota
                                            for (int y = 0; y < running_apps_[i].core_id_vec.size(); y++) {
                                                uForEstimation[running_apps_[i].core_id_vec[y]] = quotaReq * 100 / running_apps_[i].core_id_vec.size();
                                            }
                                            float powerFactorTemp = TDP / this->sensors_->estimate_power(uForEstimation, this->sensors_->getCPUCurFreq(7), this->sensors_->getCPUCurFreq(0));
                                            if (powerFactorTemp >= 1) {
                                                running_apps_[i].perf_estimate = running_apps_[i].perf_factor * quotaReq / running_apps_[i].quota;
                                                CGUtils::UpdateCpuQuota(running_apps_[i].pid, quotaReq);
                                                this->profile_ = false;
                                                this->decide_ = false;
                                                this->refine_ = true;
                                                running_apps_[i].quota = quotaReq;
                                            }
                                            else {//new quota has resulted in power violation, so make no changes and try APPX
                                                uForEstimation = uTemp;
                                                //since util failed, switch to APPX
                                                int level = 100 * (1 - running_apps_[i].perf_estimate);
                                                level = 10 * ((level + 5) / 10);
                                                if (level > 100) { level = 50; }
                                                else if (level < 0) { level = 0; }
                                                data_t* applSHM = monitorPtrRead(this->appl_monitor_->appls[running_apps_[i].app_id].segmentId);
                                                if (running_apps_[i].apx_level + level < 100 && running_apps_[i].apx_level + level >= 0)
                                                {
                                                    running_apps_[i].apx_level += level;
                                                }
                                                setPrecisionLevel(applSHM, running_apps_[i].apx_level);
                                                this->profile_ = false;
                                                this->decide_ = false;
                                                this->refine_ = true;
                                            }
                                        }
                                    }
                                }
                                else { //performance is higher than required, so scale down utilization.
                                    float quotaReq = this->running_apps_[i].quota / this->running_apps_[i].perf_factor;
                                    running_apps_[i].perf_estimate = running_apps_[i].perf_factor * quotaReq / running_apps_[i].quota;
                                    CGUtils::UpdateCpuQuota(running_apps_[i].pid, quotaReq);
                                    this->profile_ = false;
                                    this->decide_ = false;
                                    this->refine_ = true;
                                    running_apps_[i].quota = quotaReq;
                                }
                            }
                        }
                    }
                }
                else {
                    std::cout << "==== Power < TDP ====" << std::endl;
                    //sort apps by lowest performance per util
                    // this candidate will determine the DVFS level of the cluster
                    std::sort(running_apps_.begin(), running_apps_.end(),
                        [](const rtmInfo_prapx_t& a, const rtmInfo_prapx_t& b)
                        //this represents perf-per-quota..
                        {return a.perf_factor * a.core_id_vec.size() / a.quota < b.perf_factor* b.core_id_vec.size() / b.quota; }
                    );
                    bool dopDone = false;
                    bool dvfsDone = false;
                    bool dopFail = false;
                    for (int i = 0; i < this->running_apps_.size(); i++) {
                        bool migDone = false;
                        //bool dopFail = false;
                        bool goToDvfs = false;
                        bool goToApx = false;
                        bool dopDvfsFlag = false;
                        float headroom;
                        if (this->running_apps_[i].perf_factor < 1) {
                            //keep track of free cores, to be used for DoP or Task mig..
                            std::cout << "==== Pf < 1 ====" << std::endl;

                            if (this->running_apps_[i].appl.is_parallel && running_apps_[i].core_id_vec.front() > 3) {    //Option1: DOP on big cores
                                std::cout << "Option 1: DOP on big core" << std::endl;
                                std::vector<int> freeBigCores;
                                std::vector<int> dopCores;
                                for (int u = 4; u < 8; u++) {
                                    if (this->mapping_configuration_[u] == 0) {
                                        freeBigCores.push_back(u);
                                    }
                                }
                                if (freeBigCores.size()) {
                                    //there are free cores for the app to scale out..
                                    for (int t = 0; t < freeBigCores.size(); t++) {
                                        running_apps_[i].core_id_vec.push_back(freeBigCores[t]);
                                    }
                                    //data_t* applSHM = monitorPtrRead(this->appl_monitor_->appls[running_apps_[i].app_id].segmentId);
                                    this->running_apps_[i].num_threads = running_apps_[i].core_id_vec.size();
                                    this->running_apps_[i].quota = running_apps_[i].core_id_vec.size();
                                    //setnum_threads(applSHM, running_apps_[i].num_threads);
                                    CGUtils::UpdateCpuSetParallel(this->running_apps_[i].pid, running_apps_[i].core_id_vec);
                                    //update global mapping configuration
                                    for (int u = 0; u < running_apps_[i].core_id_vec.size(); u++) {
                                        this->mapping_configuration_[running_apps_[i].core_id_vec[u]] = 1;
                                    }
                                    //since changed DoP and thus mapping, come back to DECIDE phase again..
                                    this->decide_ = true;
                                    this->profile_ = false;
                                    this->refine_ = false;
                                    this->running_apps_[i].dop_flag = true;
                                    dopDone = true;
                                    dopFail = false;
                                }
                                else {
                                    std::cout << "DOP Failed for Parallel App. Go for DVFS" << std::endl;
                                    goToDvfs = true;
                                }
                            }
                            else {
                                std::cout << "Option 2: DVFS" << std::endl;
                                goToDvfs = true;
                            }
                            if (goToDvfs && !dvfsDone) {
                                int f1 = this->sensors_->getCPUCurFreq(running_apps_[i].core_id_vec.front());
                                //int f2 = (f1 / running_apps_[i].perf_factor * running_apps_[i].core_id_vec.size()) / running_apps_[i].quota;
                                //Increment the set frequency by one step
                                int f2 = f1 + 100;
                                //Safety Checks
                                if (running_apps_[i].core_id_vec.front() < 4) {
                                    if (f2 > FMAX_LITTLE) { f2 = FMAX_LITTLE; }
                                }
                                else {
                                    if (f2 > FMAX_BIG) { f2 = FMAX_BIG; }
                                }
                                if (f2 < FMIN) { f2 = FMIN; }
                                //Check headroom to see if DVFS is feasible
                                if (f2 != f1) {
                                    headroom = TDP / this->sensors_->estimate_power(uForEstimation, f2, this->sensors_->getCPUCurFreq(0));
                                    std::cout << "headroom: " << headroom << std::endl;
                                    if (headroom > 1) {
                                        int start = 0;
                                        int end = 0;
                                        std::cout << "Updating frequency to: " << f2 << " at core: " << running_apps_[i].core_id_vec.front() << std::endl;
                                        this->sensors_->setCPUFreq(running_apps_[i].core_id_vec.front(), f2);
                                        this->running_apps_[i].dvfs_flag = true;
                                        if (this->running_apps_[i].core_id_vec.front() < 4) { dvfsDoneL = true; }
                                        else { dvfsDoneB = true; }
                                    }
                                    else {
                                        std::cout << "DVFS headroom limit reached. Going to Option 2: APX" << std::endl;
                                        goToApx = true;
                                    }
                                }
                                else {
                                    std::cout << "DVFS limit reached. Going to Option 2: APX" << std::endl;
                                    goToApx = true;
                                }
                            }
                            if (goToApx) {
                                std::cout << "Into Perf APX" << std::endl;
                                int level = 100 * (1 - running_apps_[i].perf_estimate);
                                level = 10 * ((level + 5) / 10);
                                if (level > 100) level = 50;
                                else if (level < 0) level = 0;
                                data_t* applSHM = monitorPtrRead(this->appl_monitor_->appls[running_apps_[i].app_id].segmentId);
                                if (running_apps_[i].apx_level + level < 100 && running_apps_[i].apx_level + level >= 0)
                                    running_apps_[i].apx_level += level;
                                setPrecisionLevel(applSHM, running_apps_[i].apx_level);
                                this->profile_ = false;
                                this->decide_ = false;
                                this->refine_ = true;
                            }
                        }
                        else {
                            if (!dvfsDoneB || !dvfsDoneL) {
                                std::cout << "==== Pf > 1 === Check DVFS to scale down power ====" << std::endl;
                                int f1 = this->sensors_->getCPUCurFreq(running_apps_[i].core_id_vec.front());
                                //int f2 = f1 / (running_apps_[i].perf_factor * running_apps_[i].core_id_vec.size() / running_apps_[i].quota);
                                int f2 = f1 - 100;
                                //Safety Checks
                                if (running_apps_[i].core_id_vec.front() < 4) {
                                    if (f2 > FMAX_LITTLE) { f2 = FMAX_LITTLE; }
                                }
                                else {
                                    if (f2 > FMAX_BIG) { f2 = FMAX_BIG; }
                                }
                                if (f2 < FMIN) { f2 = FMIN; }
                                if (f2 != f1) {
                                    headroom = TDP / this->sensors_->estimate_power(uForEstimation, f2, this->sensors_->getCPUCurFreq(0));
                                    if (headroom > 1) {
                                        std::cout << "====DVFS === SCALE DOWN PWR ====" << std::endl;
                                        this->sensors_->setCPUFreq(running_apps_[i].core_id_vec.front(), f2);
                                        running_apps_[i].dvfs_flag = true;
                                        dvfsDone = true;
                                        this->running_apps_[i].dvfs_flag = true;
                                        if (this->running_apps_[i].core_id_vec.front() < 4) { dvfsDoneL = true; }
                                        else { dvfsDoneB = true; }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (!this->profile_ && !this->decide_ && this->refine_) {
                std::cout << "=========REFINE==========" << std::endl;
                float powerNow = this->sensors_->getBigW() + this->sensors_->getLittleW();
                // =================== Power Violated =============
                if (powerNow >= TDP) {
                    std::cout << "TDP Violation" << std::endl;
                    this->decide_ = true;
                    this->refine_ = false;
                    this->profile_ = false;
                }
                else {
                    for (int i = 0; i < running_apps_.size(); i++) {
                        if (running_apps_[i].perf_factor > 1) {
                            std::cout << "=========pf > 1==========" << std::endl;
                            float quotaReq = this->running_apps_[i].quota / this->running_apps_[i].perf_factor;
                            std::vector<int> uTemp = uForEstimation;
                            //keep a copy of util; this will be the uForEstimation back again, in case power factor >1 with new quota
                            for (int y = 0; y < running_apps_[i].core_id_vec.size(); y++) {
                                uForEstimation[running_apps_[i].core_id_vec[y]] = quotaReq * 100 / running_apps_[i].core_id_vec.size();
                            }
                            running_apps_[i].perf_estimate = running_apps_[i].perf_factor * quotaReq / running_apps_[i].quota;
                            std::cout << "Update Quota" << std::endl;
                            CGUtils::UpdateCpuQuota(running_apps_[i].pid, quotaReq);
                            running_apps_[i].quota = quotaReq;
                            this->profile_ = false;
                            this->decide_ = false;
                            this->refine_ = true;
                        }
                        else if (running_apps_[i].perf_factor < 1) {
                            std::cout << "=========pf < 1==========" << std::endl;
                            if (running_apps_[i].quota < running_apps_[i].core_id_vec.size()) {
                                float quotaReq = this->running_apps_[i].quota / this->running_apps_[i].perf_factor;
                                std::vector<int> uTemp = uForEstimation;
                                //keep a copy of util; this will be the uForEstimation back again, in case power factor >1 with new quota
                                for (int y = 0; y < running_apps_[i].core_id_vec.size(); y++) {
                                    uForEstimation[running_apps_[i].core_id_vec[y]] = quotaReq * 100 / running_apps_[i].core_id_vec.size();
                                }
                                float powerFactorTemp = TDP / this->sensors_->estimate_power(uForEstimation, this->sensors_->getCPUCurFreq(7), this->sensors_->getCPUCurFreq(0));
                                if (powerFactorTemp >= 1) {
                                    std::cout << "Option: 1 Update Quota" << std::endl;
                                    running_apps_[i].perf_estimate = running_apps_[i].perf_factor * quotaReq / running_apps_[i].quota;
                                    CGUtils::UpdateCpuQuota(running_apps_[i].pid, quotaReq);
                                    running_apps_[i].quota = quotaReq;
                                    this->profile_ = false;
                                    this->decide_ = false;
                                    this->refine_ = true;
                                }
                                else {
                                    //since util failed, switch to APPX
                                    std::cout << "Option: 2 APX" << std::endl;
                                    int level = 100 * (1 - running_apps_[i].perf_estimate);
                                    level = 10 * ((level + 5) / 10);
                                    if (level > 100) level = 50;
                                    else if (level < 0) level = 0;
                                    data_t* applSHM = monitorPtrRead(this->appl_monitor_->appls[running_apps_[i].app_id].segmentId);
                                    if (running_apps_[i].apx_level + level < 100 && running_apps_[i].apx_level + level >= 0)
                                        running_apps_[i].apx_level += level;
                                    setPrecisionLevel(applSHM, running_apps_[i].apx_level);
                                    this->profile_ = false;
                                    this->decide_ = false;
                                    this->refine_ = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    printAttachedApplications(this->appl_monitor_);
    printThroughput(this->appl_monitor_);
    printPower();
    printUtilization(u);
    printFreq();

    std::cout << "Detached apps: " << this->appl_monitor_->nDetached << std::endl;
    std::cout << "Temperatures (bigs and GPU): ";
    for (int j = 0; j < 5; j++) {
        std::cout << sensors_->getCPUTemp(j) << " ";
    }
    std::cout << std::endl;
    std::cout << "Power consumption: " << sensors_->getBigW() << " (big) " << sensors_->getLittleW() << " (LITTLE)" << std::endl;
    std::cout << "- last 5 sec mean: " << sensors_->getBigWavg() << " (big) " << sensors_->getLittleWavg() << " (LITTLE)" << std::endl;
    std::cout << "CPU usage: ";
    for (int j = 0; j < u.size(); j++) {
        std::cout << u[j] << " ";
    }
    std::cout << std::endl;
    std::cout << "Current Mapping: ";
    for (int j = 0; j < 8; j++) {
        std::cout << this->mapping_configuration_[j] << " ";
    }
    std::cout << std::endl;
    std::cout << "FREQ0: " << this->sensors_->getCPUCurFreq(0) << " FREQ7: " << this->sensors_->getCPUCurFreq(7) << std::endl;
}

void ParallelAppxPolicy::printThroughput(monitor_t* monitor) {
    //  perfLog << appl_monitor_->appls[app_id].name << "\t" << getCurrThroughput(&data) << std::endl;
    for (int j = 0; j < monitor->nAttached; j++) {
        data_t data = monitorRead(monitor->appls[j].segmentId);
        perfLog << monitor->appls[j].name << "\t" << monitor->appls[j].pid << "\t" << getCurrThroughput(&data) << "\t";
    }
    perfLog << std::endl;
}

void ParallelAppxPolicy::printAccuracy(std::vector<rtmInfo_prapx_t> appsList) {
    //	for(int i=0; i< appsList.size(); i++){
    //		apxLog << appsList[i].pid << "\t" << appsList[i].apx_level << std::endl;
    //	}
    //	apxLog << std::endl;
    return;
}

void ParallelAppxPolicy::printPower() {
    powerLog << sensors_->getBigW() + sensors_->getLittleW() << std::endl;
}

void ParallelAppxPolicy::printUtilization(std::vector<int> u) {
    for (int j = 0; j < u.size(); j++)
        utilLog << u[j] << " ";
    utilLog << std::endl;
}

void ParallelAppxPolicy::printFreq() {
    for (int j = 0; j < 8; j++)
        freqLog << sensors_->getCPUCurFreq(j) << "\t";
    freqLog << std::endl;
}
