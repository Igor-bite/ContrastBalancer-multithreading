//
// Created by Igor Kluzhev on 03.10.2024.
//

#include "time_monitor.h"
#include <omp.h>

using namespace std;

TimeMonitor::TimeMonitor(int threadsNum, bool autoPrintOnStop) {
    this->threadsNum = threadsNum;
    this->autoPrintOnStop = autoPrintOnStop;
}

void TimeMonitor::start() {
    start_time = omp_get_wtime();
    isActive = true;
}

double TimeMonitor::stop() {
    double end_time = omp_get_wtime();
    double elapsedTime;
    if (isActive) {
        isActive = false;
        elapsedTime = (end_time - start_time) * 1000;
        printf("Time (%i threads): %lg\n", threadsNum, elapsedTime);
        return elapsedTime;
    }
    return 0;
}