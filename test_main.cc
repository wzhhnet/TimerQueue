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

void nowtime(const std::string &str)
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm *now_tm = std::localtime(&now_time_t);
    std::cout << str << std::put_time(now_tm, "%H:%M:%S") << std::endl;
}

class SampleTimer : public ITimer
{
  public:
    SampleTimer(int sec)
        : str_("The time after " + std::to_string(sec) + " seconds: "),
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

int main(int argc, char **argv)
{
    using std::chrono::duration;
    using std::chrono::duration_cast;

    auto cb1 = [](ITimer *timer) { nowtime("The time after 1 second:  "); };
    auto cb2 = [](ITimer *timer) { nowtime("The time after 2 seconds: "); };
    auto cb3 = [](ITimer *timer) { nowtime("The time after 3 seconds: "); };

    nowtime("Current time:             ");
    TimerNs dtn1 = duration_cast<TimerNs>(TimerSec(1));
    TimerNs dtn2 = duration_cast<TimerNs>(TimerSec(2));
    TimerNs dtn3 = duration_cast<TimerNs>(TimerSec(3));
    TimerHandle hdl = std::make_shared<SampleTimer>(4);
    auto &tq = TimerQueue::GetInstance();
    auto th1 = tq.AddTimer(dtn1, cb1);
    auto th2 = tq.AddTimer(dtn2, cb2);
    auto th3 = tq.AddTimer(dtn3, cb3);
    tq.AddTimer(hdl);
    sleep(5);

    return 0;
}
