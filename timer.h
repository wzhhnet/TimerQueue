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

#include <chrono>

namespace utils
{

class ITimer;
using TimerNs = std::chrono::nanoseconds;
using TimerMs = std::chrono::milliseconds;
using TimerUs = std::chrono::microseconds;
using TimerSec = std::chrono::seconds;
using TimerHandle = std::shared_ptr<ITimer>;
using TimerClock = std::chrono::steady_clock;
using TimePoint = std::chrono::steady_clock::time_point;

/// @brief Abstract timer class
class ITimer
{
  public:
    /// @brief Callback on time out
    /// @return next time-out point for cycle timer
    ///         "NOW" for no-cycle timer
    virtual TimePoint TimerCallback() = 0;

    /// @brief Get time point
    /// @return time point
    virtual const TimePoint &TimerPoint() const = 0;
};

/// @brief Comparison of TimerHandle
struct TimerCompare {
    bool operator()(const TimerHandle &lhs, const TimerHandle &rhs) const
    {
        return lhs->TimerPoint() < rhs->TimerPoint();
    }
};

/// @brief An implemented class of ITimer
class Timer final : public ITimer
{
  public:
    /// @brief Timer constructor
    /// @tparam T type of time duiration
    /// @tparam F type of callable object
    /// @tparam ...Args type of parameters for F
    /// @param duration time duration
    /// @param max maximum cyclic times, at least 1 time
    /// @param func callable object
    /// @param ...args parameters for func
    template <class T, class F, class... Args>
    Timer(size_t max, T &&duration, F &&func, Args &&...args)
    {
        Bind(std::forward<F>(func), std::forward<Args>(args)...);
        max_count_ = max;
        duration_ =
            std::chrono::duration_cast<TimerNs>(std::forward<T>(duration));
        tp_ = TimerClock::now() + duration_;
    }

    /// @brief Callback override
    /// @return next time-out point for cycle timer
    ///         "NOW" for no-cycle timer
    virtual TimePoint TimerCallback() override
    {
        {
            std::unique_lock<std::mutex> lck(mtx_);
            task_();
        }
        if (++count_ < max_count_) {
            tp_ = TimerClock::now() + duration_;
        }
        return tp_;
    }

    /// @brief TimerPoint override
    /// @return time point
    virtual const TimePoint &TimerPoint() const override { return tp_; };

  private:
    /// @brief bind function and arguments to task
    /// @tparam F callable type
    /// @tparam ...Args type of function's arguments
    /// @param func ref of callable object
    /// @param ...args ref of arguments
    template <class F, class... Args> void Bind(F &&func, Args &&...args)
    {
        std::unique_lock<std::mutex> lck(mtx_);
#if __cplusplus >= 202002L // C++20 Perfect forward by "pack init-capture"
        task_ = [f = std::forward<F>(func),
                 ... args = std::forward<Args>(args)]() mutable {
            std::invoke(f, std::forward<Args>(args)...);
        };
#elif __cplusplus >= 201703L // C++17 Perfect forward by std::tuple
        task_ = [f = std::forward<F>(func),
                 args =
                     std::make_tuple(std::forward<Args>(args)...)]() mutable {
            std::apply(std::move(f), std::move(args));
        };
#else // C++11 Only copy args... type of rvalue-ref can not passed compiling.
       task_ = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
#endif
    }

  private:
    size_t count_;
    size_t max_count_;
    TimerNs duration_;
    TimePoint tp_;
    std::mutex mtx_;
    std::function<void()> task_;
};

/// @brief Construct an object of derived class of ITimer
/// @tparam T type of derived class of ITimer
/// @tparam ...Args type of arguments
/// @param ...args arguments of constructor
/// @return timer handle
template <class T, class... Args> std::shared_ptr<T> MakeTimer(Args &&...args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/// @brief Construct an object of Timer
/// @tparam T type of time duiration
/// @tparam F type of callable object
/// @tparam ...Args type of parameters
/// @param max maximum cyclic times, at least 1 time
/// @param time duration
/// @param func callable object
/// @param ...args arguments
/// @return timer handle
template <class T, class F, class... Args>
std::shared_ptr<Timer> MakeTimer(T &&time, size_t max, F &&func, Args &&...args)
{
    return std::make_shared<Timer>(max, std::forward<T>(time),
                                   std::forward<F>(func),
                                   std::forward<Args>(args)...);
}

} // namespace utils