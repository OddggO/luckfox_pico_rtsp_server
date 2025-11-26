#include "ThreadPool.h"
#include "Log.h"

ThreadPool::Task::Task(): mTaskCallback(nullptr), mArg(nullptr)
{}

ThreadPool::Task::Task(TaskCallback cb, void* arg): mTaskCallback(cb), mArg(arg)
{}

void ThreadPool::Task::handleCallback()
{
    if (mTaskCallback)
        mTaskCallback(mArg);
}

bool ThreadPool::Task::operator=(const Task& task)
{
    if (this == &task)
        return true;
    mTaskCallback = task.mTaskCallback;
    mArg = task.mArg;
    return true;
}

ThreadPool* ThreadPool::createNew(int num)
{
    return new ThreadPool(num);
}

ThreadPool::ThreadPool(int num): mThreads(num), mQuit(false)
{
    createThreads();
}

ThreadPool::~ThreadPool()
{
    cancleThreads();
}

void ThreadPool::createThreads()
{
    std::lock_guard<std::mutex> lck(mTaskQMtx);
    // for(auto t : mThreads)
    for(auto& t : mThreads)
    {
        t.start(this);
    }
}

void ThreadPool::cancleThreads()
{
    std::lock_guard<std::mutex> lck(mTaskQMtx);
    mQuit = true;
    mTaskQCon.notify_all();
    for(auto& t : mThreads)
    {
        t.join();
    }
    mThreads.clear();
}

void ThreadPool::addTask(Task& task)
{
    std::lock_guard<std::mutex> lck(mTaskQMtx);
    mTaskQueue.push(task);
    mTaskQCon.notify_one();
}

void ThreadPool::loop()
{
    while (!mQuit)
    {
        Task task;
        { // 限定锁的作用域
            std::unique_lock<std::mutex> lck(mTaskQMtx);
#if 0
            while (!mQuit && mTaskQueue.empty())
            {
                mTaskQCon.wait(lck);
            }
#else
            // 使用条件变量的谓词版本，避免虚假唤醒的问题，与上面的代码等价
            mTaskQCon.wait(lck, [this]() {
                return mQuit || !mTaskQueue.empty();
            });
#endif
            if (mQuit) {
                LOGI("thread pool thread quit");
                break;
            }
            task = std::move(mTaskQueue.front());
            mTaskQueue.pop();
        }
        // 在锁外执行任务，避免长时间持有锁
        task.handleCallback();
    }
}

void ThreadPool::MThread::run()
{
    ThreadPool* pool = (ThreadPool*)mArg;
    pool->loop();
}
