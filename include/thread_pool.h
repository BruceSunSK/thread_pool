#pragma once
#include <iostream>
#include <thread>
#include <atomic>
#include <future>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>


class ThreadPool
{
public:
    ThreadPool(int min_thread = 2, int max_thread = std::thread::hardware_concurrency());
    ~ThreadPool();

    template <typename F, typename ... Args>
    auto add_task(F && f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using ReturnType = typename std::result_of<F(Args...)>::type;

        std::shared_ptr<std::packaged_task<ReturnType()>> task =
            std::make_shared<std::packaged_task<ReturnType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<ReturnType> future = task->get_future();
        tasks_mutex_.lock();
        tasks_.push([task]() { (*task)(); });
        tasks_mutex_.unlock();
        continue_condition_.notify_one();
        return future;
    }

private:
    std::thread manager_;                                       // 管理者线程
    std::unordered_map<std::thread::id, std::thread> workers_;  // 工作线程
    std::vector<std::thread::id> ids_;                          // 存储需要销毁的线程id
    std::queue<std::function<void()>> tasks_;                   // 存储待解决的任务

    std::mutex tasks_mutex_;
    std::mutex ids_mutex_;
    std::condition_variable_any continue_condition_;
    std::condition_variable_any delete_condition_;

    std::atomic<int> min_thread_;
    std::atomic<int> max_thread_;
    std::atomic<int> curr_thread_;
    std::atomic<int> idle_thread_;
    std::atomic<int> exit_thread_;
    std::atomic<bool> stop_flag_;

    void manager_thread();
    void worker_thread();
};

