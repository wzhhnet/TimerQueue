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
    Timer(TimerNs dtn, TimerFunc func, bool safe);
    Timer(TimePoint &tp, TimerFunc func, bool safe);
    virtual ~Timer();
    virtual void TimerCallback() override { func_(this); }
    virtual const TimePoint &TimerPoint() const override { return tp_; };

  private:
    TimePoint tp_;
    TimerFunc func_;
};

bool TimerHandleComp::operator()(const TimerHandle &lhs,
                                 const TimerHandle &rhs) const
{
    if (lhs != nullptr && rhs != nullptr) {
        return lhs->TimerPoint() < rhs->TimerPoint();
    } else {
        return false;
    }
}

Timer::Timer(TimerNs dtn, TimerFunc func, bool safe)
    : ITimer(safe), tp_(TimerClock::now() + dtn), func_(func)
{
}

Timer::Timer(TimePoint &tp, TimerFunc func, bool safe)
    : ITimer(safe), tp_(tp), func_(func)
{
}

Timer::~Timer() {}

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
    if (tq_.end() != tq_.find(handle)) {
        return false;
    }
    std::unique_lock<std::mutex> lck(mtx_);
    tq_.emplace(handle);
    cv_.notify_one();
    return true;
}

TimerHandle TimerQueue::AddTimer(TimePoint &tp, TimerFunc func, bool safe)
{
    if (tp < TimerClock::now()) {
        return TimerHandle();
    }
    auto handle = std::make_shared<Timer>(tp, func, safe);
    std::unique_lock<std::mutex> lck(mtx_);
    tq_.emplace(handle);
    cv_.notify_one();
    return handle;
}

TimerHandle TimerQueue::AddTimer(TimerNs dtn, TimerFunc func, bool safe)
{
    auto handle = std::make_shared<Timer>(dtn, func, safe);
    std::unique_lock<std::mutex> lck(mtx_);
    tq_.emplace(handle);
    cv_.notify_one();
    return handle;
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
        /// User need hold TimerHanle for safety!
        if (!hdl->Safe() || !hdl.unique()) {
            hdl->TimerCallback();
        }
    }
    return true;
}

} // namespace utils
