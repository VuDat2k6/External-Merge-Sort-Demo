#pragma once
#include <vector>

struct DemoRun {
    std::vector<std::vector<double>> pages; // mỗi page là 1 vector giá trị
};

struct DemoFrameCursor {
    int runIndex = -1;     // run nào đang được nạp vào frame
    int pageIndex = -1;    // page nào của run
    int recordIndex = -1;  // con trỏ vào record trong page
};

struct DemoState {
    std::vector<double> input;

    // Runs của pass hiện tại
    std::vector<DemoRun> currentRuns;

    // Runs đang được tạo ra trong pass hiện tại
    std::vector<DemoRun> nextRuns;

    // 3 input frames: F1, F2, F3
    std::vector<DemoFrameCursor> frames;

    // Output frame (1 page)
    std::vector<double> outputFrame;

    // Kết quả cuối
    std::vector<double> finalOutput;

    int passNumber = 0;
    int groupNumber = 0;

    // Để animation chạy đúng
    bool hasPending = false;
    double pendingValue = 0.0;
    int pendingFrame = -1; // 0..2
};