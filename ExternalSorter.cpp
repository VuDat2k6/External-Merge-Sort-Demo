#include "ExternalSorter.h"

#include <fstream>
#include <vector>
#include <algorithm>
#include <queue>
#include <cstdio>
#include <stdexcept>
#include <cstdint>
#include <filesystem>
#include <ctime>
#include <cmath>

namespace fs = std::filesystem;

static void safeRemove(const std::string& p) {
    std::error_code ec;
    fs::remove(fs::path(p), ec);
}

static std::string makeTempName(const fs::path& dir,
    const std::string& prefix,
    std::size_t idx,
    const std::string& ext = ".bin")
{
    return (dir / fs::path(prefix + "_" + std::to_string(idx) + ext)).string();
}

std::uint64_t ExternalSorter::getFileSizeBytes(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("Cannot open input file.");
    return static_cast<std::uint64_t>(f.tellg());
}

void ExternalSorter::sortDoublesBinary(const std::string& inputPath,
    const std::string& outputPath,
    std::size_t chunkMB,
    LogFn log,
    ProgressFn progress)
{
    if (chunkMB == 0)
        throw std::runtime_error("Chunk size must be > 0.");

    auto L = [&](const std::string& s) {
        if (log) log(s);
        };

    auto P = [&](int p) {
        if (!progress) return;
        if (p < 0) p = 0;
        if (p > 100) p = 100;
        progress(p);
        };

    fs::path tmpDir = fs::temp_directory_path();
    const std::string prefix =
        "extsort_" + std::to_string(static_cast<std::uint64_t>(std::time(nullptr)));

    const std::uint64_t totalBytes = getFileSizeBytes(inputPath);
    if (totalBytes % sizeof(double) != 0)
        throw std::runtime_error("Input file size is not a multiple of 8 bytes (double).");

    const std::uint64_t totalCount = totalBytes / sizeof(double);

    L("Input: " + std::to_string(totalBytes) +
        " bytes (" + std::to_string(totalCount) + " doubles)\n");

    if (totalCount == 0) {
        std::ofstream out(outputPath, std::ios::binary);
        if (!out)
            throw std::runtime_error("Cannot create output file.");
        P(100);
        L("Done (empty input).\n");
        return;
    }

    const std::uint64_t chunkBytes =
        static_cast<std::uint64_t>(chunkMB) * 1024ULL * 1024ULL;
    const std::uint64_t chunkCap =
        std::max<std::uint64_t>(1, chunkBytes / sizeof(double));

    L("Phase 1: split + sort chunks\n");
    L("Chunk capacity: " + std::to_string(chunkCap) + " doubles\n");

    std::ifstream in(inputPath, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open input stream.");

    std::vector<std::string> runs;
    runs.reserve(static_cast<std::size_t>((totalCount + chunkCap - 1) / chunkCap));

    std::vector<double> buf;
    buf.reserve(static_cast<std::size_t>(chunkCap));

    std::uint64_t readCount = 0;
    std::size_t runIdx = 0;

    while (readCount < totalCount) {
        const std::uint64_t toRead =
            std::min<std::uint64_t>(chunkCap, totalCount - readCount);

        buf.assign(static_cast<std::size_t>(toRead), 0.0);

        in.read(reinterpret_cast<char*>(buf.data()),
            static_cast<std::streamsize>(toRead * sizeof(double)));
        if (!in)
            throw std::runtime_error("Read error while reading input.");

        std::sort(buf.begin(), buf.end());

        const std::string runPath = makeTempName(tmpDir, prefix + "_run", runIdx);
        std::ofstream runOut(runPath, std::ios::binary);
        if (!runOut)
            throw std::runtime_error("Cannot create temp run: " + runPath);

        runOut.write(reinterpret_cast<const char*>(buf.data()),
            static_cast<std::streamsize>(buf.size() * sizeof(double)));
        if (!runOut)
            throw std::runtime_error("Write error in temp run: " + runPath);

        runs.push_back(runPath);
        readCount += toRead;
        runIdx++;

        P(static_cast<int>((readCount * 50) / totalCount));
        L("  Wrote run " + std::to_string(runIdx) + ": " + runPath + "\n");
    }

    auto mergeGroup = [&](const std::vector<std::string>& group, const std::string& outRun) {
        struct Node {
            double value;
            std::size_t fileIdx;
            bool operator>(const Node& other) const {
                return value > other.value;
            }
        };

        std::vector<std::ifstream> ins(group.size());
        for (std::size_t i = 0; i < group.size(); i++) {
            ins[i].open(group[i], std::ios::binary);
            if (!ins[i])
                throw std::runtime_error("Cannot open temp run for merge: " + group[i]);
        }

        std::ofstream out(outRun, std::ios::binary);
        if (!out)
            throw std::runtime_error("Cannot create merged run: " + outRun);

        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;

        for (std::size_t i = 0; i < ins.size(); i++) {
            double x;
            if (ins[i].read(reinterpret_cast<char*>(&x), sizeof(double))) {
                pq.push(Node{ x, i });
            }
        }

        while (!pq.empty()) {
            Node n = pq.top();
            pq.pop();

            out.write(reinterpret_cast<const char*>(&n.value), sizeof(double));
            if (!out)
                throw std::runtime_error("Write error during merge: " + outRun);

            double x;
            if (ins[n.fileIdx].read(reinterpret_cast<char*>(&x), sizeof(double))) {
                pq.push(Node{ x, n.fileIdx });
            }
        }
        };

    const std::size_t FAN_IN = 32;
    L("Phase 2: multi-pass merge (fan-in = " + std::to_string(FAN_IN) + ")\n");

    std::vector<std::string> current = std::move(runs);
    std::vector<std::string> allTemps = current;

    std::size_t pass = 0;

    while (current.size() > 1) {
        pass++;
        std::vector<std::string> next;

        const std::size_t groupsTotal =
            static_cast<std::size_t>(std::ceil(current.size() / static_cast<double>(FAN_IN)));
        std::size_t groupsDone = 0;

        L("  Merge pass " + std::to_string(pass) +
            ": " + std::to_string(current.size()) + " runs\n");

        for (std::size_t start = 0; start < current.size(); start += FAN_IN) {
            const std::size_t end = std::min(start + FAN_IN, current.size());

            std::vector<std::string> group(current.begin() + start, current.begin() + end);

            const std::string outRun =
                makeTempName(tmpDir, prefix + "_merge" + std::to_string(pass), next.size());

            mergeGroup(group, outRun);

            for (const auto& g : group) {
                safeRemove(g);
            }

            next.push_back(outRun);
            allTemps.push_back(outRun);

            groupsDone++;
            int p = 50 + static_cast<int>((groupsDone * 45) /
                std::max<std::size_t>(1, groupsTotal));
            P(p);
        }

        current = std::move(next);
    }

    if (current.empty())
        throw std::runtime_error("Unexpected error: no final run generated.");

    const std::string finalRun = current.front();

    {
        fs::path outPath(outputPath);
        if (outPath.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(outPath.parent_path(), ec);
        }
    }

    {
        std::error_code ec;
        fs::remove(fs::path(outputPath), ec);

        fs::rename(fs::path(finalRun), fs::path(outputPath), ec);
        if (ec) {
            fs::copy_file(fs::path(finalRun),
                fs::path(outputPath),
                fs::copy_options::overwrite_existing,
                ec);

            if (ec)
                throw std::runtime_error("Cannot write output file: " + outputPath);

            safeRemove(finalRun);
        }
    }

    for (const auto& t : allTemps) {
        if (t != outputPath) {
            safeRemove(t);
        }
    }

    P(100);
    L("Done. Output: " + outputPath + "\n");

}
