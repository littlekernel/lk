#include "semaphore.h"

namespace NDebug {


Semaphore::Semaphore(int count) : count_(count) {}

void Semaphore::signal() {
    std::unique_lock<std::mutex> lck(mtx_);
    ++count_;
    cv_.notify_one();
}

void Semaphore::wait() {
    std::unique_lock<std::mutex> lck(mtx_);
    while(count_ == 0) {
      cv_.wait(lck);
    }

    --count_;
}

}  // namespace NDebug