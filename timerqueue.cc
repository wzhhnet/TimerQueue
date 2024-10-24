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

class Timer : public ITimer
{
  public:
    Timer(TimerNs dtn, TimerFunc func)
        : ITimer(), tp_(TimerClock::now() + dtn), func_(func)
    {
    }
    Timer(TimePoint &tp, TimerFunc func) : ITimer(), tp_(tp), func_(func) {}
    virtual ~Timer() {}
    virtual void TimerCallback() override { func_(this); }
    virtual const TimePoint &TimerPoint() const override { return tp_; };

  private:
    TimePoint tp_;
    TimerFunc func_;
};

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

TimerHandle TimerQueue::AddTimer(TimePoint &tp, TimerFunc func)
{
    if (tp < TimerClock::now()) {
        return TimerHandle();
    }
    auto handle = std::make_shared<Timer>(tp, func);
    std::unique_lock<std::mutex> lck(mtx_);
    tq_.emplace(handle);
    cv_.notify_one();
    return handle;
}

TimerHandle TimerQueue::AddTimer(TimerNs dtn, TimerFunc func)
{
    auto handle = std::make_shared<Timer>(dtn, func);
    std::unique_lock<std::mutex> lck(mtx_);
    tq_.emplace(handle);
    cv_.notify_one();
    return handle;
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

bool TimerQueue::RemoveTimer(const TimePoint &tp)
{
    bool rc = false;
    std::unique_lock<std::mutex> lck(mtx_);
    auto it = tq_.begin();
    while (it != tq_.end()) {
        if (tp == (*it)->TimerPoint()) {
            it = tq_.erase(it);
            rc = true;
        } else
            it++;
    }
    return rc;
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
        } else {
            auto it = tq_.begin();
            if (*it == nullptr) {
                tq_.erase(it);
                return true;
            }
            auto st = cv_.wait_until(lck, (*it)->TimerPoint());
            if (st == std::cv_status::timeout) {
                hdl = *it;
                tq_.erase(it);
            } else {
                // AddTimer or quit invoked
                return true;
            }
        }
    }
    if (hdl != nullptr) {
        hdl->TimerCallback();
    }
    return true;
}

} // namespace utils
