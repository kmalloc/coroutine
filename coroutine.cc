#include "coroutine.h"

#include <map>
#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>

using namespace std;

struct coroutine
{
    void* arg;

    int status;
    ucontext_t cxt;
    uintptr_t yield;
    CoroutineScheduler::CoFunc func;

    char stack[0];
};

class CoroutineScheduler::SchedulerImpl
{
    public:

        explicit SchedulerImpl(int stacksize);
        ~SchedulerImpl();

        int CreateCoroutine(CoroutineScheduler::CoFunc func, void* arg);
        int DestroyCoroutine(int id);

        uintptr_t Yield(uintptr_t);
        uintptr_t ResumeCoroutine(int id, uintptr_t);

        bool IsCoroutineAlive(int id) const;

    private:

        SchedulerImpl(const SchedulerImpl&);
        SchedulerImpl& operator=(const SchedulerImpl&);

        static void Schedule(void* arg);

    private:

        int index_;
        int running_;
        const int stacksize_;

        ucontext_t mainContext_;
        map<int, coroutine*> id2routine_;
        coroutine** routine_;
};

CoroutineScheduler::SchedulerImpl::SchedulerImpl(int stacksize)
    :index_(0), running_(-1), stacksize_(stacksize)
{
}

CoroutineScheduler::SchedulerImpl::~SchedulerImpl()
{
    map<int, coroutine*>::iterator it = id2routine_.begin();

    while (it != id2routine_.end())
    {
        if (it->second) free (it->second);
    }
}

bool CoroutineScheduler::SchedulerImpl::IsCoroutineAlive(int id) const
{
    map<int, coroutine*>::const_iterator it = id2routine_.find(id);
    if (it == id2routine_.end()) return false;

    return it->second;
}

int CoroutineScheduler::SchedulerImpl::DestroyCoroutine(int id)
{
    coroutine* cor = id2routine_[id];
    if (!cor) return -1;

    free(cor);
    id2routine_.erase(id);
    return id;
}

// static function
void CoroutineScheduler::SchedulerImpl::Schedule(void* arg)
{
    assert(arg);
    SchedulerImpl* sched = (SchedulerImpl*) arg;

    int running = sched->running_;

    coroutine* cor = sched->id2routine_[running];
    assert(cor);

    cor->yield = cor->func(cor->arg);

    sched->running_ = -1;
    cor->status = CO_FINISHED;
}

int CoroutineScheduler::SchedulerImpl::CreateCoroutine(CoroutineScheduler::CoFunc func, void* arg)
{
    coroutine* cor = (coroutine*)malloc(sizeof(coroutine) + stacksize_);

    if (cor == NULL) return -1;

    cor->arg  = arg;
    cor->func = func;
    cor->yield = 0;
    cor->status = CO_READY;

    int index = index_++;
    id2routine_[index] = cor;

    return index;
}

uintptr_t CoroutineScheduler::SchedulerImpl::ResumeCoroutine(int id, uintptr_t y)
{
    coroutine* cor = id2routine_[id];
    if (cor == NULL || cor->status == CO_RUNNING) return 0;

    cor->yield = y;
    switch (cor->status)
    {
        case CO_READY:
            {
                getcontext(&cor->cxt);

                cor->status = CO_RUNNING;
                cor->cxt.uc_stack.ss_sp = cor->stack;
                cor->cxt.uc_stack.ss_size = stacksize_;
                // sucessor context.
                cor->cxt.uc_link = &mainContext_;

                running_ = id;
                // setup coroutine context
                makecontext(&cor->cxt, (void (*)())Schedule, 1, this);
                swapcontext(&mainContext_, &cor->cxt);
            }
            break;
        case CO_SUSPENDED:
            {
                running_ = id;
                cor->status = CO_RUNNING;
                swapcontext(&mainContext_, &cor->cxt);
            }
            break;
        default:
            assert(0);
    }

    uintptr_t ret = cor->yield;
    cor->yield = 0;

    if (running_ == -1 && cor->status == CO_FINISHED) DestroyCoroutine(id);

    return ret;
}

uintptr_t CoroutineScheduler::SchedulerImpl::Yield(uintptr_t y)
{
    if (running_ < 0) return 0;

    int cur = running_;
    running_ = -1;

    coroutine* cor = id2routine_[cur];

    cor->yield = y;
    cor->status = CO_SUSPENDED;

    swapcontext(&cor->cxt, &mainContext_);

    uintptr_t ret = cor->yield;
    cor->yield = 0;
    return ret;
}

CoroutineScheduler::CoroutineScheduler(int stacksize)
    :impl_(new SchedulerImpl(stacksize))
{
}

CoroutineScheduler::~CoroutineScheduler()
{
    delete impl_;
}

int CoroutineScheduler::CreateCoroutine(CoFunc func, void* arg)
{
    return impl_->CreateCoroutine(func, arg);
}

int CoroutineScheduler::DestroyCoroutine(int id)
{
    return impl_->DestroyCoroutine(id);
}

uintptr_t CoroutineScheduler::ResumeCoroutine(int id, uintptr_t y)
{
    return impl_->ResumeCoroutine(id, y);
}

uintptr_t CoroutineScheduler::Yield(uintptr_t y)
{
    return impl_->Yield(y);
}

bool CoroutineScheduler::IsCoroutineAlive(int id) const
{
    return impl_->IsCoroutineAlive(id);
}

