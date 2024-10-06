//
// Created by Igor Kluzhev on 03.10.2024.
//

#include "time_monitor.h"
#include <iostream>
#include <omp.h>

using namespace std;

TimeMonitor::TimeMonitor(string label, bool autoPrintOnStop) {
    this->label = label;
    this->autoPrintOnStop = autoPrintOnStop;
}

void TimeMonitor::start() {
    start_time = omp_get_wtime();
    isActive = true;
}

int TimeMonitor::stop() {
    double end_time = omp_get_wtime();
    int elapsedTime;
    if (isActive) {
        isActive = false;
        elapsedTime = int((end_time - start_time) * 1000);
    } else {
        elapsedTime = -1;
    }
    cout << "[" + label + "] Time: " << elapsedTime << "ms" << endl;
    return elapsedTime;
}