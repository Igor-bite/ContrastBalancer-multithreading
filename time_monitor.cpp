//
// Created by Igor Kluzhev on 03.10.2024.
//

#include "time_monitor.h"
#include <iostream>
#include <omp.h>

using namespace std;

TimeMonitor::TimeMonitor() = default;

void TimeMonitor::start() {
    start_time = omp_get_wtime();
    isActive = true;
}

int TimeMonitor::stop() {
    double end_time = omp_get_wtime();
    if (isActive) {
        isActive = false;
        return int((end_time - start_time) * 1000);
    } else {
        return -1;
    }
}