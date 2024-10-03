//
// Created by Igor Kluzhev on 03.10.2024.
//

#include "time_monitor.h"
#include <ctime>

using namespace std;

TimeMonitor::TimeMonitor() = default;

void TimeMonitor::start() {
    start_time = clock();
    isActive = true;
}

double TimeMonitor::stop() {
    auto end_time = clock();
    if (isActive) {
        isActive = false;
        return static_cast<double>(end_time - start_time) / CLOCKS_PER_SEC;
    } else {
        return -1;
    }
}