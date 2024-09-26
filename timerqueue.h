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

#pragma once

#include <set>
#include <mutex>
#include <chrono>
#include <thread>
#include <functional>
#include <condition_variable>

#include "singleton.h"

namespace utils {

class TimerQueue;
class ITimer;

using TimerNs = std::chrono::nanoseconds;
using TimerMs = std::chrono::milliseconds;
using TimerUs = std::chrono::microseconds;
using TimerSec = std::chrono::seconds;
using TimerHandle = std::shared_ptr<ITimer>;
using TimerClock = std::chrono::steady_clock;
using TimerFunc = std::function<void(ITimer*)>;
using TimePoint = std::chrono::steady_clock::time_point;
using TimerDuration = std::chrono::steady_clock::duration;

/// Less timepoint of timer in the front of TimerQueue
struct TimerHandleComp
{
    bool operator() (
        const TimerHandle& lhs, const TimerHandle& rhs) const;
};

/// Abstract timer
class ITimer
  : public std::enable_shared_from_this<ITimer>
{
  public:
    ITimer(bool safe = true) : safe_(safe) {}
    virtual ~ITimer() {}
    virtual void TimerCallback() = 0;
    virtual const TimePoint& TimerPoint() const = 0;
    bool Safe() const { return safe_; }

  private:
    bool safe_;
    TimerFunc func_ = &ITimer::TimerCallback;
};

/// Timer container as a singleton managered all user timers
class TimerQueue final : public Singleton<TimerQueue>
{
  friend Singleton<TimerQueue>;
  public:
    /// User implement ITimer and shared TimerHandle, so it is safety
    bool AddTimer(const TimerHandle& handle);

    /// User may use lambda as TimerFunc, we need to consider safety issues
    /// about lifetime of captured variables by lambda.
    ///
    /// "safe" is true:
    ///    We need user hold TimerHandle before timer expired,
    ///    if not, for safty, timer will be deleted at timepoint of timeout,
    ///    and "TimerFunc" will not be invoked.
    ///
    /// "safe" is false:
    ///    If user does't hold TimerHandle, "TimerFunc" will be always invoked
    ///    when its timer expired.
    ///    In some scenarios, ensure the lambda is enclosed and does not capture variables.
    ///    then we use it conveniently. however use it carefully!
    TimerHandle AddTimer(TimerNs dtn, TimerFunc func, bool safe = true);
    TimerHandle AddTimer(TimePoint& tp, TimerFunc func, bool safe = true);

  private:
    TimerQueue();
    virtual ~TimerQueue();
    void StartRoutine();
    bool ThreadLoop();
    void Stop();

  private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::unique_ptr<std::thread> thread_;
    std::set<TimerHandle, TimerHandleComp> tq_;
    bool quit_ = false;
};

}
