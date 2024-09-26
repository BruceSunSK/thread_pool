#include "thread_pool.h"

int add(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return a + b;
}

int main(int argc, char const * argv[])
{
    ThreadPool pool;

    std::vector<std::future<int>> futures;
    for (size_t i = 0; i < 30; i++)
    {
        futures.emplace_back(pool.add_task(add, i, i * 2));
    }

    for (size_t i = 0; i < futures.size(); i++)
    {
        printf("输出为：%d\n", futures[i].get());
    }
    return 0;
}
