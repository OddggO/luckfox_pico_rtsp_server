#pragma once
#include "Thread.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

class ThreadPool
{
public:
    class Task
    {
    public:
        typedef void (*TaskCallback)(void*);
        Task();
        Task(TaskCallback cb, void* arg);
        void handleCallback();
        // 重载赋值运算符
        // q: 为什么返回值是bool类型？a: 通常赋值运算符重载返回一个引用以支持链式赋值，但这里返回bool表示赋值是否成功。
        // q: 为什么输入参数是const引用？a: 避免不必要的拷贝，同时保证输入参数不会被修改。
        bool operator=(const Task& Task); 
    private:
        TaskCallback mTaskCallback;
        void* mArg;
    };
    static ThreadPool* createNew(int num);
    ThreadPool(int num);
    ~ThreadPool();

    void addTask(Task& task);
protected:
    void loop();
    void createThreads();
    void cancleThreads();

    class MThread: public Thread
    {
    protected:
        // virtual void run(void* arg);
        virtual void run();
    };
private:
    std::vector<MThread> mThreads;
    std::queue<Task> mTaskQueue;

    std::mutex mTaskQMtx;
    std::condition_variable mTaskQCon;
    bool mQuit;
};
