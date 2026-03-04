#pragma once
#include "DemoState.h"
#include <functional>
#include <string>
#include <vector>

struct DemoStep {
    std::string description;
    std::function<void(DemoState&)> apply;
};

class DemoController {
public:
    static constexpr int K = 3;            // M-1 input frames
    static constexpr std::size_t PAGE_CAP = 2; // mỗi page chứa 2 số để dễ minh họa

    void Build(const std::vector<double>& inputValues, std::size_t chunkSizeElements = 4);

    void Reset();
    bool CanNext() const;
    bool CanPrev() const;
    void Next();
    void Prev();

    const DemoState& GetState() const;
    std::string GetDescription() const;

private:
    std::vector<DemoStep> steps;
    int stepIndex = -1;

    DemoState initialState;
    DemoState state;
    std::string currentDescription;

private:
    void RebuildTo(int idx);
};