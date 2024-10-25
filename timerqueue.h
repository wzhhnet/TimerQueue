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
#include <future>
#include <chrono>
#include <thread>
#if __cplusplus >= 201703L
#include <type_traits>
#endif
#include <functional>
#include <condition_variable>
#include "singleton.h"

namespace utils
{
/// @brief Forward declaration
class TimerQueue;
class ITimer;

using TimerNs = std::chrono::nanoseconds;
using TimerMs = std::chrono::milliseconds;
using TimerUs = std::chrono::microseconds;
using TimerSec = std::chrono::seconds;
using TimerHandle = std::shared_ptr<ITimer>;
using TimerClock = std::chrono::steady_clock;
using TimerFunc = std::function<void(ITimer *)>;
using TimePoint = std::chrono::steady_clock::time_point;

template <class F, class... Args>
#if __cplusplus >= 201703L
using Result = std::invoke_result<F, Args...>;
#elif __cplusplus >= 201103L
using Result = std::result_of<F(Args...)>;
#else
#error "c++11 or higher version must be supported"
#endif
template <class F, class... Args>
using ResultType = typename Result<F, Args...>::type;

/// @brief Abstract timer class
class ITimer : public std::enable_shared_from_this<ITimer>
{
  public:
    /// @brief Callback on time out
    virtual void TimerCallback() = 0;

    /// @brief Get time point
    /// @return time point
    virtual const TimePoint &TimerPoint() const = 0;
};

/// @brief Comparison for timer sorting.
struct TimerCompare {
    bool operator()(const TimerHandle &lhs, const TimerHandle &rhs) const
    {
        return lhs->TimerPoint() < rhs->TimerPoint();
    };
};

/// @brief Container as a singleton managered all user timers
class TimerQueue final : public Singleton<TimerQueue>
{
    friend Singleton<TimerQueue>;

  public:
    /// @brief Add timer that user implemented.
    /// @param handle reference of timer object.
    /// @return true if successfully.
    bool AddTimer(const TimerHandle &handle);

    /// @brief Add a timer to "TimerQueue".
    /// @param dtn duration between "NOW" and time-out
    /// @param func callback on time out.
    /// @return timer object handle.
    TimerHandle AddTimer(TimerNs &dtn, TimerFunc func);

    /// @brief Add a timer to "TimerQueue".
    /// @param tp time point on time out.
    /// @param func callback on time out.
    /// @return timer object handle.
    TimerHandle AddTimer(TimePoint &tp, TimerFunc func);

    /// @brief Remove a timer by handle.
    /// @param handle "TimerHandle" object ref.
    /// @return true if success.
    bool RemoveTimer(const TimerHandle &handle);

    /// @brief Remove all timers which "TimePoint" equals to tp.
    /// @param tp "TimePoint" object ref.
    /// @return ture if success.
    bool RemoveTimer(const TimePoint &tp);

    /// @brief Add a callable object to "TimerQueue"
    /// @tparam ...Args arguments for callable object
    /// @tparam T Type of duration or "TimePoint"
    /// @tparam F Type of callable object
    /// @param time Duration or "TimePoint"
    /// @param func Callable object(e.g. function, lambda ...)
    /// @param ...args Arguments for callable object
    /// @return future object associated to callable object.
    ///         Be careful with the future object, if user accesses "future" by
    ///         "future::get()" after removing the timer by "RemoveTimer", it
    ///         will cause a "Broken promise" exception.
    template <class T, class F, class... Args>
    auto AddTimerEx(T &&time, F &&func,
                    Args &&...args) -> std::future<ResultType<F, Args...>>
    {
        auto package =
            std::make_shared<std::packaged_task<ResultType<F, Args...>()>>(
                std::bind(std::forward<F>(func), std::forward<Args>(args)...));
        std::future<ResultType<F, Args...>> res = package->get_future();
        TimerFunc tf = [package](ITimer *) { (*package)(); };
        AddTimer(std::forward<T>(time), tf);
        return res;
    }

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
    std::set<TimerHandle, TimerCompare> tq_;
    bool quit_ = false;
};

} // namespace utils
