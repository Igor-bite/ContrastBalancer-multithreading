//
// Created by Igor Kluzhev on 03.10.2024.
//

#ifndef TESTPROJECT_TIME_MONITOR_H
#define TESTPROJECT_TIME_MONITOR_H

#include <string>

using namespace std;

class TimeMonitor {
public:
    explicit TimeMonitor(int threadsNum, bool autoPrintOnStop);

    void start();
    void stop();

private:
    double start_time;
    bool isActive = false;
    bool autoPrintOnStop;
    int threadsNum;
};

#endif //TESTPROJECT_TIME_MONITOR_H
