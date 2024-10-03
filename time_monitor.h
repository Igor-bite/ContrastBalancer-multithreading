//
// Created by Igor Kluzhev on 03.10.2024.
//

#ifndef TESTPROJECT_TIME_MONITOR_H
#define TESTPROJECT_TIME_MONITOR_H

class TimeMonitor {
public:
    TimeMonitor();

    void start();
    double stop();

private:
    double start_time;
    bool isActive = false;
};

#endif //TESTPROJECT_TIME_MONITOR_H
