/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include <cstdint>
#include <fstream>
#include <ios>
#include <unistd.h>

enum class MemoryUnit : uint8_t {
    Bit,
    Byte,
    Kilobyte,
    Megabyte,
    Gigabyte
};

static constexpr double BytesPerUnit(MemoryUnit unit) noexcept {
    switch (unit) {
        case MemoryUnit::Bit:      return 1.0 / 8.0;
        case MemoryUnit::Byte:     return 1.0;
        case MemoryUnit::Kilobyte: return 1024.0;
        case MemoryUnit::Megabyte: return 1024.0 * 1024.0;
        case MemoryUnit::Gigabyte: return 1024.0 * 1024.0 * 1024.0;
    }
    return 1.0;
}

static double GetMemoryUsage(MemoryUnit _unit) {
    double rssUsage = 0.0;

    std::ifstream statStream("/proc/self/stat", std::ios_base::in);
    if (!statStream)
        return 0.0;

    // dummy vars for leading entries in stat that we don't care about
    //
    std::string pid, comm, state, ppid, pgrp, session, ttyNr;
    std::string tpgid, flags, minflt, cminflt, majflt, cmajflt;
    std::string utime, stime, cutime, cstime, priority, nice;
    std::string o, itrealvalue, starttime;

    // the two fields we want
    //
    unsigned long vsize;
    long rss;

    statStream >> pid >> comm >> state >> ppid >> pgrp >> session >> ttyNr
                >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
                >> utime >> stime >> cutime >> cstime >> priority >> nice
                >> o >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

    statStream.close();

    long pageSizeBytes = sysconf(_SC_PAGE_SIZE);

    //vmUsage = static_cast<double>(vsize) / BytesPerUnit(_unit);
    rssUsage = static_cast<double>(rss * pageSizeBytes) / BytesPerUnit(_unit);
    return rssUsage;
}