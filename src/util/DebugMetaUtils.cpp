// Copyright 2023 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "util/DebugMetaUtils.h"
#include "crypto/Hex.h"
#include "crypto/Random.h"
#include "util/Fs.h"
#include <fmt/format.h>

namespace stellar
{
namespace metautils
{

std::filesystem::path
getMetaDebugDirPath(std::filesystem::path const& bucketDir)
{
    return bucketDir / META_DEBUG_DIRNAME;
}

std::filesystem::path
getMetaDebugFilePath(std::filesystem::path const& bucketDir, uint32_t seqNum)
{
    auto file =
        fmt::format(META_DEBUG_FILE_FMT_STR, seqNum, binToHex(randomBytes(8)));
    return getMetaDebugDirPath(bucketDir) / file;
}

std::vector<std::filesystem::path>
listMetaDebugFiles(std::filesystem::path const& bucketDir)
{
    auto dir = getMetaDebugDirPath(bucketDir);
    auto files = fs::findfiles(dir.string(), [](std::string const& file) {
        return std::regex_match(file, META_DEBUG_FILE_REGEX);
    });
    std::sort(files.begin(), files.end());
    return std::vector<std::filesystem::path>(files.begin(), files.end());
}

bool
isDebugSegmentBoundary(uint32_t ledgerSeq)
{
    if (META_DEBUG_LEDGER_SEGMENT_SIZE == 1)
    {
        return true;
    }
    return ledgerSeq % (META_DEBUG_LEDGER_SEGMENT_SIZE - 1) == 0;
}

size_t
getNumberOfDebugFilesToKeep(uint32_t numLedgers)
{
    size_t segLen = META_DEBUG_LEDGER_SEGMENT_SIZE;
    return (numLedgers + segLen - 1) / segLen;
}
}
}