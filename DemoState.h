#pragma once
#include <vector>

struct DemoRun {
    std::vector<std::vector<double>> pages;
};

struct DemoFrameCursor {
    int runIndex = -1;     
    int pageIndex = -1;    
    int recordIndex = -1; 
};

struct DemoState {
    std::vector<double> input;
    std::vector<DemoRun> currentRuns;
    std::vector<DemoRun> nextRuns;
    std::vector<DemoFrameCursor> frames;
    std::vector<double> outputFrame;
    std::vector<double> finalOutput;

    int passNumber = 0;
    int groupNumber = 0;

    bool hasPending = false;
    double pendingValue = 0.0;
    int pendingFrame = -1; 

};
