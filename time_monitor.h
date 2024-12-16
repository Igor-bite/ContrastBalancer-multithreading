//
// Created by Igor Kluzhev on 03.10.2024.
//

#ifndef TESTPROJECT_TIME_MONITOR_H
#define TESTPROJECT_TIME_MONITOR_H

#include <string>
#include <chrono>

using namespace std;

class TimeMonitor {
public:
    explicit TimeMonitor(bool autoPrintOnStop);

    void start();
    double stop();

private:
    chrono::steady_clock::time_point start_time;
    bool isActive = false;
    bool autoPrintOnStop;
};

#endif //TESTPROJECT_TIME_MONITOR_H
