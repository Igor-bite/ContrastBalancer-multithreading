//
// Created by Igor Kluzhev on 03.10.2024.
//

#include "time_monitor.h"
#include <chrono>

using namespace std;

TimeMonitor::TimeMonitor(int threadsNum, bool autoPrintOnStop) {
    this->threadsNum = threadsNum;
    this->autoPrintOnStop = autoPrintOnStop;
}

void TimeMonitor::start() {
    start_time = chrono::steady_clock::now();
    isActive = true;
}

double TimeMonitor::stop() {
    chrono::steady_clock::time_point end_time = chrono::steady_clock::now();
    double elapsedTime;
    if (isActive) {
        isActive = false;
        elapsedTime = double(chrono::duration_cast<chrono::microseconds>(end_time - start_time).count()) / 1000;
        printf("Time (%i threads): %lg\n", threadsNum, elapsedTime);
        return elapsedTime;
    }
    return 0;
}