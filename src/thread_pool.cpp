#include "thread_pool.h"


ThreadPool::ThreadPool(int min_thread, int max_thread) :
    min_thread_(min_thread), max_thread_(max_thread), stop_flag_(false),
    curr_thread_(min_thread), idle_thread_(min_thread),
    manager_(&ThreadPool::manager_thread, this)
{
    for (size_t i = 0; i < min_thread_; i++)
    {
        std::thread t(&ThreadPool::worker_thread, this);
        workers_.insert(std::make_pair(t.get_id(), std::move(t)));
        std::cout << "\033[32m--------- 线程：" << t.get_id() << "，已创建 ---------\033[0m\n";
    }
}

ThreadPool::~ThreadPool()
{
    stop_flag_ = true;

    continue_condition_.notify_all();
    for (auto && it : workers_)
    {
        it.second.join();
        std::cout << "\033[31m--------- 线程：" << it.first << "，已销毁 ---------\033[0m\n";
    }
    manager_.join();
}

void ThreadPool::manager_thread()
{
    while (!stop_flag_)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        int curr = curr_thread_;
        int idle = idle_thread_;
        if (idle > curr / 2 && curr > min_thread_)
        {
            // 每次销毁两个线程
            exit_thread_ = 2;
            continue_condition_.notify_all();

            std::unique_lock<std::mutex> locker(ids_mutex_);
            delete_condition_.wait(ids_mutex_);
            for (std::thread::id & id : ids_)
            {
                auto it = workers_.find(id);
                if (it != workers_.end())
                {
                    it->second.join();
                    std::cout << "\033[31m--------- 线程：" << it->first << "，已销毁 ---------\033[0m\n";
                    workers_.erase(it);
                }
            }
            ids_.clear();
        }
        else if(idle == 0 && curr < max_thread_)
        {
            std::thread t(&ThreadPool::worker_thread, this);
            workers_.insert(std::make_pair(t.get_id(), std::move(t)));
            std::cout << "\033[32m--------- 线程：" << t.get_id() << "，已创建 ---------\033[0m\n";
            curr_thread_++;
            idle_thread_++;
        }
    }
}

void ThreadPool::worker_thread()
{
    while (!stop_flag_)
    {
        std::function<void()> task = nullptr;
        std::unique_lock<std::mutex> locker(tasks_mutex_);
        while (tasks_.empty() && !stop_flag_)
        {
            continue_condition_.wait(locker);
            if (exit_thread_ > 0)
            {
                exit_thread_--;
                idle_thread_--;
                curr_thread_--;
                std::cout << "\033[33m--------- 线程：" << std::this_thread::get_id() << "，已退出 ---------\033[0m\n";
                std::unique_lock<std::mutex> locker(ids_mutex_);
                ids_.push_back(std::this_thread::get_id());

                if (exit_thread_ == 0)
                {
                    delete_condition_.notify_one();
                }
                return;
            }
        }
        if (!tasks_.empty() && !stop_flag_)
        {
            task = std::move(tasks_.front());
            tasks_.pop();
            locker.unlock();
            if (task)
            {
                idle_thread_--;
                task();
                idle_thread_++;
            }
        }
    }

    std::cout << "\033[33m--------- 线程：" << std::this_thread::get_id() << "，已退出 ---------\033[0m\n";
}