//
// Created by Igor Kluzhev on 03.10.2024.
//

#ifndef TESTPROJECT_TIME_MONITOR_H
#define TESTPROJECT_TIME_MONITOR_H

#include <string>

using namespace std;

class TimeMonitor {
public:
    explicit TimeMonitor(string label, bool autoPrintOnStop);

    void start();
    int stop();

private:
    double start_time;
    bool isActive = false;
    bool autoPrintOnStop;
    string label;
};

#endif //TESTPROJECT_TIME_MONITOR_H
