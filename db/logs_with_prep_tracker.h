#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    // This class is used to tracker the log files with outstanding prepare entries.
    class LogsWithPrepTracker
    {
    public:
        // Called when a transaction prepared in 'log' has been committed or aborted.
        void MarkLogAsHavingPrepSectionFlushed(uint64_t log);
        // Called when a transaction is prepared in 'log'
        void MarkLogAsContainingPrepSection(uint64_t log);
        // Return the earliest log file with outstanding prepare entries.
        uint64_t FindMinLogContainingOutstandingPrep();
        size_t TEST_PreparedSectionCompletedSize()
        {
            return prepared_section_completed_.size();
        }
        size_t TEST_LogsWithPrepSize() { return logs_with_prep_.size(); }

    private:
        struct LogCnt
        {
            uint64_t log; // the log number
            uint64_t cnt; // number of prepared sections in the log
        };
        std::vector<LogCnt> logs_with_prep_; // 一个有序列表，存储仍然包含准备数据的日志文件
        std::mutex logs_with_prep_mutex_;

        std::unordered_map<uint64_t, uint64_t> prepared_section_completed_; // 用于跟踪已经完成的准备部分
        std::mutex prepared_section_completed_mutex_;
    };
}