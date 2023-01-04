// Copyright (C) 2018-2022 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions
// and limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

template <class T>
class SafeQueue {
  public:
    void push(T t) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(t);
        lock.unlock();
        condition_.notify_one();
    }

    void push_front(T t) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_front(t);
        lock.unlock();
        condition_.notify_one();
    }

    T &front() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            condition_.wait(lock);
        }
        return queue_.front();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            condition_.wait(lock);
        }
        T value = queue_.front();
        queue_.pop_front();
        lock.unlock();
        condition_.notify_one();
        return value;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    void waitEmpty() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            condition_.wait(lock);
        }
    }

  private:
    std::deque<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
};
