#pragma once

#include <string>
#include <functional>
#include <cstdint>

class ExternalSorter
{
public:
    using LogFn = std::function<void(const std::string&)>;
    using ProgressFn = std::function<void(int)>;

    void sortDoublesBinary(const std::string& inputPath,
        const std::string& outputPath,
        std::size_t chunkMB,
        LogFn log,
        ProgressFn progress);

    static std::uint64_t getFileSizeBytes(const std::string& path);
};