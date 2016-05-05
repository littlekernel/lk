#pragma once

#include <mutex>
#include <condition_variable>

namespace NDebug {

class Semaphore {
 public:
    Semaphore(int count);
    void signal();
    void wait();

 private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;
};

}  // namespace NDebug