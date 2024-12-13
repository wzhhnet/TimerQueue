/*
 * Test program for TimeQueue
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

#include <unistd.h>
#include <ctime>
#include <iomanip>
#include <iostream>

#include "timerqueue.h"

using namespace utils;

TimePoint nowtime(const std::string &str)
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto steady_now = steady_clock::now();
    std::time_t now_time_t = system_clock::to_time_t(now);
    std::tm *now_tm = std::localtime(&now_time_t);
    auto ms = duration_cast<TimerMs>(now.time_since_epoch()) % 1000;
    std::cout << str << std::put_time(now_tm, "%H:%M:%S.") << std::setw(3)
              << std::setfill('0') << ms.count() << std::endl;
    return steady_now;
}

class SampleTimer : public ITimer
{
  public:
    SampleTimer(int sec)
        : str_("The time after " + std::to_string(sec) + "   seconds: "),
          tp_(std::chrono::steady_clock::now() + std::chrono::seconds(sec))
    {
    }
    virtual ~SampleTimer() {}
    virtual void TimerCallback() override { nowtime(str_); }
    virtual const TimePoint &TimerPoint() const override { return tp_; }

  private:
    std::string str_;
    TimePoint tp_;
};

static float InvokeAfter(float sec)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << sec;
    auto str = "The time after " + oss.str() + " seconds: ";
    nowtime(str);
    return sec;
}

int main(int argc, char **argv)
{
    using std::chrono::duration;
    using std::chrono::duration_cast;

    auto cb1 = []() { nowtime("The time after 1   second:  "); };
    auto cb2 = []() { nowtime("The time after 2   seconds: "); };
    auto cb3 = []() { nowtime("The time after 3   seconds: "); };

    auto tp = nowtime("Current time:               ");
    auto &tq = TimerQueue::GetInstance();
    tq.AddTimer(MakeTimer(TimerSec(1), cb1));
    tq.AddTimer(MakeTimer(TimerSec(2), cb2));
    tq.AddTimer(MakeTimer(TimerSec(3), cb3));
    tq.AddTimer(MakeTimer<SampleTimer>(4));
    int ms = 5000;
    for (; ms < 6000; ms += 100)
        tq.AddTimer(MakeTimer(TimerMs(ms), InvokeAfter, ms / 1000.0));
    auto hdl = MakeTimer(TimerMs(ms), InvokeAfter, ms / 1000.0);
    tq.AddTimer(hdl);
    std::cout << hdl->get_future().get() << " seconds" << std::endl;
    return 0;
}
