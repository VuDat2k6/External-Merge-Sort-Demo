#include "DemoController.h"
#include <algorithm>
#include <stdexcept>

namespace {

    DemoRun MakeRunFromValues(const std::vector<double>& values, std::size_t pageCap)
    {
        DemoRun run;
        for (std::size_t i = 0; i < values.size(); i += pageCap) {
            const std::size_t end = std::min(i + pageCap, values.size());
            run.pages.emplace_back(values.begin() + i, values.begin() + end);
        }
        return run;
    }

    std::vector<double> FlattenRun(const DemoRun& run)
    {
        std::vector<double> out;
        for (const auto& page : run.pages) {
            out.insert(out.end(), page.begin(), page.end());
        }
        return out;
    }

    bool RunHasMoreRecords(const DemoRun& run, int pageIndex, int recordIndex)
    {
        if (pageIndex < 0 || pageIndex >= static_cast<int>(run.pages.size())) return false;
        if (recordIndex < 0 || recordIndex >= static_cast<int>(run.pages[pageIndex].size())) return false;

        if (recordIndex + 1 < static_cast<int>(run.pages[pageIndex].size())) return true;
        if (pageIndex + 1 < static_cast<int>(run.pages.size())) return true;
        return false;
    }

} 

void DemoController::Build(const std::vector<double>& inputValues, std::size_t chunkSizeElements)
{
    if (inputValues.empty()) throw std::runtime_error("Input file is empty.");
    if (inputValues.size() > 20) throw std::runtime_error("Demo mode only supports <= 20 doubles.");
    if (chunkSizeElements == 0) throw std::runtime_error("chunkSizeElements must be > 0.");

    steps.clear();
    stepIndex = -1;

    initialState = DemoState{};
    initialState.input = inputValues;
    initialState.frames.assign(K, DemoFrameCursor{});

    std::vector<std::vector<double>> rawChunks;
    for (std::size_t i = 0; i < inputValues.size(); i += chunkSizeElements) {
        const std::size_t end = std::min(i + chunkSizeElements, inputValues.size());
        rawChunks.emplace_back(inputValues.begin() + i, inputValues.begin() + end);
    }

    std::vector<DemoRun> runsPass0;
    runsPass0.reserve(rawChunks.size());
    for (const auto& chunk : rawChunks) {
        runsPass0.push_back(MakeRunFromValues(chunk, PAGE_CAP));
    }

    {
        auto runsSnapshot = runsPass0;
        steps.push_back({
            "Pass 0: Chia file thành các run ban đầu (chưa sắp xếp).",
            [runsSnapshot](DemoState& s) {
                s.passNumber = 0;
                s.groupNumber = 0;
                s.currentRuns = runsSnapshot;
                s.nextRuns.clear();
                s.frames.assign(DemoController::K, DemoFrameCursor{});
                s.outputFrame.clear();
                s.finalOutput.clear();
                s.hasPending = false;
                s.pendingValue = 0.0;
                s.pendingFrame = -1;
            }
            });
    }

    // Sắp xếp từng chunk -> run ban đầu đã sort
    std::vector<std::vector<double>> sortedChunks = rawChunks;
    for (std::size_t i = 0; i < sortedChunks.size(); ++i) {
        std::sort(sortedChunks[i].begin(), sortedChunks[i].end());

        std::vector<DemoRun> runsSnapshot;
        runsSnapshot.reserve(sortedChunks.size());
        for (const auto& chunk : sortedChunks) {
            runsSnapshot.push_back(MakeRunFromValues(chunk, PAGE_CAP));
        }

        steps.push_back({
            "Pass 0: Sắp xếp run R" + std::to_string(i + 1) + ".",
            [runsSnapshot](DemoState& s) {
                s.passNumber = 0;
                s.groupNumber = 0;
                s.currentRuns = runsSnapshot;
                s.frames.assign(DemoController::K, DemoFrameCursor{});
                s.outputFrame.clear();
                s.hasPending = false;
                s.pendingValue = 0.0;
                s.pendingFrame = -1;
            }
            });
    }

    std::vector<DemoRun> passRuns;
    passRuns.reserve(sortedChunks.size());
    for (const auto& chunk : sortedChunks) {
        passRuns.push_back(MakeRunFromValues(chunk, PAGE_CAP));
    }

    bool finalStepAdded = false;
    int passNo = 1;
    
    while (passRuns.size() > 1) {
        std::vector<DemoRun> plannedNextRuns;
        int groupNo = 1;

        for (std::size_t start = 0; start < passRuns.size(); start += K, ++groupNo) {
            const std::size_t end = std::min(start + static_cast<std::size_t>(K), passRuns.size());
            const std::size_t groupSize = end - start;

            std::vector<DemoRun> groupRuns(passRuns.begin() + start, passRuns.begin() + end);

            struct LocalCursor {
                int page = 0;
                int rec = 0;
                bool active = true;
            };

            std::vector<LocalCursor> local(groupSize);
            for (auto& c : local) {
                c.page = 0;
                c.rec = 0;
                c.active = true;
            }

            DemoRun builtRun;
            std::vector<double> outBuf;

            {
                auto currentRunsSnapshot = passRuns;
                steps.push_back({
                    "Pass " + std::to_string(passNo) + ", Group " + std::to_string(groupNo) +
                    ": Chọn tối đa 3 run và nạp page đầu tiên vào F1..F3.",
                    [currentRunsSnapshot, start, groupSize, passNo, groupNo](DemoState& s) {
                        s.passNumber = passNo;
                        s.groupNumber = groupNo;
                        s.currentRuns = currentRunsSnapshot;

                        s.frames.assign(DemoController::K, DemoFrameCursor{});
                        for (std::size_t f = 0; f < groupSize; ++f) {
                            s.frames[f].runIndex = static_cast<int>(start + f);
                            s.frames[f].pageIndex = 0;
                            s.frames[f].recordIndex = 0;
                        }

                        s.outputFrame.clear();
                        s.hasPending = false;
                        s.pendingValue = 0.0;
                        s.pendingFrame = -1;
                    }
                    });
            }

            steps.push_back({
                "Tạo run đầu ra mới cho group hiện tại.",
                [](DemoState& s) {
                    s.nextRuns.push_back(DemoRun{});
                    s.outputFrame.clear();
                }
                });
            
            while (true) {
                int bestLocal = -1;
                double bestValue = 0.0;

                for (std::size_t f = 0; f < groupSize; ++f) {
                    if (!local[f].active) continue;

                    const auto& run = groupRuns[f];
                    const auto& page = run.pages[static_cast<std::size_t>(local[f].page)];
                    double v = page[static_cast<std::size_t>(local[f].rec)];

                    if (bestLocal < 0 || v < bestValue) {
                        bestLocal = static_cast<int>(f);
                        bestValue = v;
                    }
                }

                if (bestLocal < 0) {
                    break; 
                }

                const int bestFrame = bestLocal;

                steps.push_back({
                    "Tìm phần tử nhỏ nhất trong các pointed records.",
                    [passNo, groupNo, bestFrame, bestValue](DemoState& s) {
                        s.passNumber = passNo;
                        s.groupNumber = groupNo;
                        s.hasPending = true;
                        s.pendingValue = bestValue;
                        s.pendingFrame = bestFrame;
                    }
                    });

                steps.push_back({
                    "Di chuyển phần tử nhỏ nhất vào output frame.",
                    [](DemoState&) {
                    }
                    });

                const auto oldPage = local[static_cast<std::size_t>(bestLocal)].page;
                const auto oldRec = local[static_cast<std::size_t>(bestLocal)].rec;

                bool stillActive = false;
                int newPage = -1;
                int newRec = -1;

                const auto& currentRun = groupRuns[static_cast<std::size_t>(bestLocal)];
                if (RunHasMoreRecords(currentRun, oldPage, oldRec)) {
                    if (oldRec + 1 < static_cast<int>(currentRun.pages[static_cast<std::size_t>(oldPage)].size())) {
                        stillActive = true;
                        newPage = oldPage;
                        newRec = oldRec + 1;
                    }
                    else {
                        stillActive = true;
                        newPage = oldPage + 1;
                        newRec = 0;
                    }
                }

                steps.push_back({
                    "Ghi phần tử vào output frame và cập nhật pointer tương ứng.",
                    [bestFrame, bestValue, stillActive, newPage, newRec](DemoState& s) {
                        s.outputFrame.push_back(bestValue);

                        s.hasPending = false;
                        s.pendingValue = 0.0;
                        s.pendingFrame = -1;

                        if (bestFrame >= 0 && bestFrame < static_cast<int>(s.frames.size())) {
                            if (stillActive) {
                                s.frames[static_cast<std::size_t>(bestFrame)].pageIndex = newPage;
                                s.frames[static_cast<std::size_t>(bestFrame)].recordIndex = newRec;
                            }
 else {
  s.frames[static_cast<std::size_t>(bestFrame)] = DemoFrameCursor{};
}
}
}
                    });

                // cập nhật mô phỏng local
                outBuf.push_back(bestValue);
                if (stillActive) {
                    local[static_cast<std::size_t>(bestLocal)].page = newPage;
                    local[static_cast<std::size_t>(bestLocal)].rec = newRec;
                    local[static_cast<std::size_t>(bestLocal)].active = true;
                }
                else {
                    local[static_cast<std::size_t>(bestLocal)].active = false;
                }

                if (outBuf.size() == PAGE_CAP) {
                    const auto pageToFlush = outBuf;
                    builtRun.pages.push_back(pageToFlush);
                    outBuf.clear();

                    steps.push_back({
                        "Output frame đầy: ghi output frame ra secondary storage.",
                        [pageToFlush](DemoState& s) {
                            if (!s.nextRuns.empty()) {
                                s.nextRuns.back().pages.push_back(pageToFlush);
                            }
                            s.outputFrame.clear();
                        }
                        });
                }
            }

            if (!outBuf.empty()) {
                const auto pageToFlush = outBuf;
                builtRun.pages.push_back(pageToFlush);
                outBuf.clear();

                steps.push_back({
                    "Kết thúc group: ghi phần còn lại của output frame ra secondary storage.",
                    [pageToFlush](DemoState& s) {
                        if (!s.nextRuns.empty()) {
                            s.nextRuns.back().pages.push_back(pageToFlush);
                        }
                        s.outputFrame.clear();
                    }
                    });
            }

            plannedNextRuns.push_back(builtRun);

            // Kết thúc group
            steps.push_back({
                "Kết thúc group hiện tại.",
                [](DemoState& s) {
                    s.frames.assign(DemoController::K, DemoFrameCursor{});
                    s.outputFrame.clear();
                    s.hasPending = false;
                    s.pendingValue = 0.0;
                    s.pendingFrame = -1;
                }
                });
        }

        if (plannedNextRuns.size() > 1) {
            steps.push_back({
                "Kết thúc pass hiện tại: dùng các run mới làm input cho pass tiếp theo.",
                [passNo](DemoState& s) {
                    s.passNumber = passNo + 1;
                    s.groupNumber = 0;
                    s.currentRuns = s.nextRuns;
                    s.nextRuns.clear();
                    s.frames.assign(DemoController::K, DemoFrameCursor{});
                    s.outputFrame.clear();
                    s.hasPending = false;
                    s.pendingValue = 0.0;
                    s.pendingFrame = -1;
                }
                });
        }
        else {
            steps.push_back({
                "Hoàn tất: chỉ còn 1 run cuối cùng.",
                [passNo](DemoState& s) {
                    s.passNumber = passNo;
                    s.groupNumber = 0;
                    s.currentRuns = s.nextRuns;
                    s.frames.assign(DemoController::K, DemoFrameCursor{});
                    s.outputFrame.clear();
                    s.hasPending = false;
                    s.pendingValue = 0.0;
                    s.pendingFrame = -1;

                    s.finalOutput.clear();
                    if (!s.currentRuns.empty()) {
                        s.finalOutput = FlattenRun(s.currentRuns.front());
                    }

                    s.nextRuns.clear();
                }
                });

            finalStepAdded = true;
        }

        passRuns = plannedNextRuns;
        passNo++;
    }

    if (!finalStepAdded && passRuns.size() == 1) {
        const auto onlyRun = passRuns.front();
        steps.push_back({
            "Hoàn tất: file đã được sắp xếp.",
            [onlyRun](DemoState& s) {
                s.currentRuns = { onlyRun };
                s.nextRuns.clear();
                s.frames.assign(DemoController::K, DemoFrameCursor{});
                s.outputFrame.clear();
                s.hasPending = false;
                s.pendingValue = 0.0;
                s.pendingFrame = -1;
                s.finalOutput = FlattenRun(onlyRun);
            }
            });
    }

    Reset();
}

void DemoController::Reset()
{
    stepIndex = -1;
    state = initialState;
    currentDescription = "Sẵn sàng minh họa K-way external merge sort.";
}

bool DemoController::CanNext() const
{
    return stepIndex + 1 < static_cast<int>(steps.size());
}

bool DemoController::CanPrev() const
{
    return stepIndex > 0;
}

void DemoController::RebuildTo(int idx)
{
    state = initialState;
    currentDescription.clear();

    for (int i = 0; i <= idx; ++i) {
        steps[i].apply(state);
        currentDescription = steps[i].description;
    }
}

void DemoController::Next()
{
    if (!CanNext()) return;
    ++stepIndex;
    RebuildTo(stepIndex);
}

void DemoController::Prev()
{
    if (!CanPrev()) return;
    --stepIndex;
    if (stepIndex < 0) Reset();
    else RebuildTo(stepIndex);
}

const DemoState& DemoController::GetState() const
{
    return state;
}

std::string DemoController::GetDescription() const
{
    return currentDescription;

}
