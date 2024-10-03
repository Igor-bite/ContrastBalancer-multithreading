//
// Created by Igor Kluzhev on 03.10.2024.
//

#ifndef TESTPROJECT_TIME_MONITOR_H
#define TESTPROJECT_TIME_MONITOR_H

#include <ctime>

class TimeMonitor {
public:
    TimeMonitor();

    void start();
    double stop();

private:
    clock_t start_time;
    bool isActive = false;
};

#endif //TESTPROJECT_TIME_MONITOR_H
