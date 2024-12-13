/*
 * Author wanch
 * Date 2023/4/10
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

namespace utils
{

template <typename T> class Singleton
{
  public:
    template <typename... Args> static T &GetInstance(Args &&...args)
    {
        static T instance(std::forward<Args>(args)...);
        return instance;
    }

    Singleton(T &&) = delete;
    Singleton(const T &) = delete;
    Singleton &operator=(T &&) = delete;
    Singleton &operator=(const T &) = delete;

  protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

}; // namespace utils
