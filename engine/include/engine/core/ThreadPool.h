#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace se {

class ThreadPool {
   public:
    explicit ThreadPool(size_t num_threads = 0) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 4;
        }

        running_ = true;

        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] { return !running_ || !tasks_.empty(); });

                        if (!running_ && tasks_.empty()) { return; }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    task();
                    ++completed_tasks_;
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            running_ = false;
        }
        condition_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) { worker.join(); }
        }
    }

    // Submit a task and get a future to wait on
    template <typename F, typename... Args>
    auto Submit(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace([task]() { (*task)(); });
        }

        condition_.notify_one();
        return result;
    }

    // Submit multiple tasks and wait for all to complete
    template <typename F>
    void ParallelFor(size_t start, size_t end, F&& func) {
        if (end <= start) return;

        size_t total       = end - start;
        size_t num_threads = workers_.size();
        size_t chunk_size  = (total + num_threads - 1) / num_threads;

        std::vector<std::future<void>> futures;
        futures.reserve(num_threads);

        for (size_t t = 0; t < num_threads; ++t) {
            size_t chunk_start = start + t * chunk_size;
            size_t chunk_end   = std::min(chunk_start + chunk_size, end);

            if (chunk_start >= end) break;

            futures.push_back(Submit([&func, chunk_start, chunk_end] {
                for (size_t i = chunk_start; i < chunk_end; ++i) { func(i); }
            }));
        }

        // Wait for all chunks to complete
        for (auto& f : futures) { f.get(); }
    }

    size_t GetThreadCount() const {
        return workers_.size();
    }
    size_t GetCompletedTasks() const {
        return completed_tasks_;
    }

   private:
    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        queue_mutex_;
    std::condition_variable           condition_;
    std::atomic<bool>                 running_{false};
    std::atomic<size_t>               completed_tasks_{0};
};

}  // namespace se
