#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class BoundedBlockingQueue {
public:
    void push(const T &item)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.push(item);
            lock.unlock();
        }
        cv_.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            cv_.wait(lock);
        }
        auto result = queue_.front();
        queue_.pop();
        return result;
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};