/*
 * TimerQueue
 * Implemented by C++
 *
 * Author wanch
 * Date 2023/10/20
 * Email wzhhnet@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "timerqueue.h"

namespace utils
{

TimerQueue::TimerQueue()
    : thread_(new std::thread(&TimerQueue::StartRoutine, this))
{
}

TimerQueue::~TimerQueue() { Stop(); }

void TimerQueue::Stop()
{
    {
        std::unique_lock<std::mutex> l(mtx_);
        quit_ = true;
        cv_.notify_one();
    }
    /// Waiting thread quit
    if (thread_ != nullptr) {
        thread_->join();
    }
}

bool TimerQueue::AddTimer(const TimerHandle &handle)
{
    if (handle == nullptr) {
        return false;
    }
    if (handle->TimerPoint() < TimerClock::now()) {
        return false;
    }
    std::unique_lock<std::mutex> lck(mtx_);
    if (tq_.end() != tq_.find(handle)) {
        return false;
    }
    tq_.emplace(handle);
    cv_.notify_one();
    return true;
}

bool TimerQueue::RemoveTimer(const TimerHandle &handle)
{
    std::unique_lock<std::mutex> lck(mtx_);
    auto it = tq_.find(handle);
    if (it == tq_.end()) {
        return false;
    }
    tq_.erase(it);
    return true;
}

void TimerQueue::StartRoutine() { while (ThreadLoop()); }

bool TimerQueue::ThreadLoop()
{
    TimerHandle hdl;
    {
        std::unique_lock<std::mutex> lck(mtx_);
        if (quit_) {
            tq_.clear();
            return false;
        }
        if (tq_.empty()) {
            cv_.wait(lck);
            return true;
        }
        auto it = tq_.begin();
        if (*it == nullptr) {
            tq_.erase(it);
            return true;
        }
        auto st = cv_.wait_until(lck, (*it)->TimerPoint());
        if (st != std::cv_status::timeout) {
            return true; // AddTimer or quit invoked
        }
        /// tp timeout
        hdl = *it;
        tq_.erase(it);
    }
    if (hdl != nullptr) {
        auto tp = hdl->TimerCallback();
        if (tp > TimerClock::now()) AddTimer(hdl);
    }
    return true;
}

} // namespace utils
