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

#include <future>
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

template <class F, class... Args>
#if __cplusplus >= 201703L
using ReturnType = typename std::invoke_result<F, Args...>::type;
#elif __cplusplus >= 201103L
using ReturnType = typename std::result_of<F(Args...)>::type;
#else
#error "c++11 or higher version must be supported"
#endif

/// @brief Abstract timer class
class ITimer
{
  public:
    /// @brief Callback on time out
    virtual void TimerCallback() = 0;

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
/// @tparam R type of future result
template <class R = void> class Timer final : public ITimer
{
  public:
    /// @brief Timer constructor
    /// @tparam T type of time duiration
    /// @tparam F type of callable object
    /// @tparam ...Args type of parameters for F
    /// @param duration time duration
    /// @param func callable object
    /// @param ...args parameters for func
    template <class T, class F, class... Args>
    Timer(T &&duration, F &&func, Args &&...args)
    {
#if __cplusplus >= 202002L // C++20 Perfect forward by "pack init-capture"
        auto task = [f = std::forward<F>(func),
                     ... args = std::forward<Args>(args)]() mutable {
            return std::invoke(f, std::forward<Args>(args)...);
        };
#elif __cplusplus >= 201703L // C++17 Perfect forward by std::tuple
        auto task =
            [f = std::forward<F>(func),
             args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(std::move(f), std::move(args));
            };
#else // C++11 Only copy args... type of rvalue-ref can not passed compiling.
        auto task =
            std::bind(std::forward<F>(func), std::forward<Args>(args)...);
#endif
        task_ = std::packaged_task<R()>(std::move(task));
        tp_ = TimerClock::now() + std::forward<T>(duration);
    }
    /// @brief Callback override
    virtual void TimerCallback() override { task_(); }

    /// @brief TimerPoint override
    /// @return time point
    virtual const TimePoint &TimerPoint() const override { return tp_; };

    /// @brief get a future
    /// @return future object associated to callable object.
    ///         Be careful with the future object, if user accesses "future" by
    ///         "future::get()" after removing the timer by "RemoveTimer", it
    ///         will cause a "Broken promise" exception.
    std::future<R> get_future() { return task_.get_future(); }

  private:
    TimePoint tp_;
    std::packaged_task<R()> task_;
};

/// @brief Construct an object of derived class of ITimer
/// @tparam T type of derived class of ITimer
/// @tparam ...Args type of arguments for constructor
/// @param ...args arguments for constructor
/// @return TimerHandle with an object of derived class of ITimer
template <class T, class... Args> std::shared_ptr<T> MakeTimer(Args &&...args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/// @brief Construct an object of Timer
/// @tparam T type of time duiration
/// @tparam F type of callable object
/// @tparam ...Args type of parameters for F
/// @param time duration
/// @param func callable object
/// @param ...args parameters for func
/// @return TimerHandle with an object of Timer
template <class T, class F, class... Args>
std::shared_ptr<Timer<ReturnType<F, Args...>>> MakeTimer(T &&time, F &&func,
                                                         Args &&...args)
{
    return std::make_shared<Timer<ReturnType<F, Args...>>>(
        std::forward<T>(time), std::forward<F>(func),
        std::forward<Args>(args)...);
}

} // namespace utils