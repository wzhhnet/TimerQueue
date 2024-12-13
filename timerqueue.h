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
#include <thread>
#include <functional>
#include <condition_variable>
#include "singleton.h"
#include "timer.h"

namespace utils
{

/// @brief Container as a singleton managered all user timers
class TimerQueue final : public Singleton<TimerQueue>
{
    friend Singleton<TimerQueue>;

  public:
    /// @brief Add timer that user implemented.
    /// @param handle reference of timer object.
    /// @return true if successfully.
    bool AddTimer(const TimerHandle &handle);

    /// @brief Remove a timer by handle.
    /// @param handle "TimerHandle" object ref.
    /// @return true if success.
    bool RemoveTimer(const TimerHandle &handle);

    /// @brief Remove all timers which "TimePoint" equals to tp.
    /// @param tp "TimePoint" object ref.
    /// @return ture if success.
    bool RemoveTimer(const TimePoint &tp);

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
