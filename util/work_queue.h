#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>

#include "xiaodb/xiaodb_namespace.h"

namespace XIAODB_NAMESPACE
{
    template <typename T>
    class WorkQueue
    {
        std::mutex mutex_;
        std::condition_variable readerCv_;
        std::condition_variable writeCv_;
        std::condition_variable finishCv_;

        std::queue<T> queue_;
        bool done_;
        std::size_t maxSize_;

        // Must have lock to call this function
        bool full() const
        {
            if (maxSize_ == 0)
            {
                return false;
            }
            return queue_.size() >= maxSize_;
        }

    public:
        WorkQueue(std::size_t maxSize = 0) : done_(false), maxSize_(maxSize) {}

        template <typename U>
        bool push(U &&item)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                while (full() && !done_)
                {
                    writeCv_.wait(lock);
                }
                if (done_)
                {
                    return false;
                }
                queue_.push(std::forward<U>(item));
            }
            readerCv_.notify_one();
            return true;
        }

        /**
         * Attempts to pop an item off the work queue.  It will block until data is
         * available or `finish()` has been called.
         *
         * @param[out] item  If `pop` returns `true`, it contains the popped item.
         *                    If `pop` returns `false`, it is unmodified.
         * @returns          True upon success.  False if the queue is empty and
         *                    `finish()` has been called.
         */
        bool pop(T &item)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                while (queue_.empty() && !done_)
                {
                    readerCv_.wait(lock);
                }
                if (queue_.empty())
                {
                    assert(done_);
                    return false;
                }
                item = queue_.front();
                queue_.pop();
            }
            writerCv_.notify_one();
            return true;
        }

        /**
         * Sets the maximum queue size.  If `maxSize == 0` then it is unbounded.
         *
         * @param maxSize The new maximum queue size.
         */
        void setMaxSize(std::size_t maxSize)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                maxSize_ = maxSize;
            }
            writerCv_.notify_all();
        }

        /**
         * Promise that `push()` won't be called again, so once the queue is empty
         * there will never any more work.
         */
        void finish()
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                assert(!done_);
                done_ = true;
            }
            readerCv_.notify_all();
            writerCv_.notify_all();
            finishCv_.notify_all();
        }

        /// Blocks until `finish()` has been called (but the queue may not be empty).
        void waitUntilFinished()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (!done_)
            {
                finishCv_.wait(lock);
            }
        }
    };
}