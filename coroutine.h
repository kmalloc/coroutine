#ifndef H_COROUTINE_H_
#define H_COROUTINE_H_

#include <stdint.h>

class CoroutineScheduler // non copyable
{
    public:

        typedef uintptr_t (*CoFunc)(void* arg);

        enum Status
        {
            CO_READY,
            CO_SUSPENDED,
            CO_RUNNING,
            CO_FINISHED
        };

    public:

        explicit CoroutineScheduler(int stacksize = 1024);
        ~CoroutineScheduler();

        int DestroyCoroutine(int id);
        int CreateCoroutine(CoFunc func, void* arg);

        uintptr_t Yield(uintptr_t y = 0);
        uintptr_t ResumeCoroutine(int id, uintptr_t y = 0);

        bool IsCoroutineAlive(int id) const;

    private:

        CoroutineScheduler(const CoroutineScheduler&);
        CoroutineScheduler& operator=(const CoroutineScheduler&);

    private:

        class SchedulerImpl;
        SchedulerImpl* impl_;
};

#endif

