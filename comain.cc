#include "coroutine.h"

#include <iostream>

using namespace std;

CoroutineScheduler* sched = NULL;

void func1(void* arg)
{
    uintptr_t ret;
    cout << "function1 a now!,arg:" << arg << ", start to yield." << endl;
    ret = sched->Yield((uintptr_t)"func1 yield 1");
    cout << "1.fun1 return from yield:" << (const char*)ret << endl;
    ret = sched->Yield((uintptr_t)"func1 yield 2");
    cout << "2.fun1 return from yield:" << (const char*)ret << ", going to stop" << endl;

}

void func2(void* s)
{
    cout << "function2 a now!, arg:" << s << ", start to yield." << endl;
    const char* y = (const char*)sched->Yield((uintptr_t)"func2 yield 1");
    cout << "fun2 return from yield:" << y <<", going to stop" << endl;
}

int main()
{
    sched = new CoroutineScheduler();

    bool stop = false;
    int f1 = sched->CreateCoroutine(func1, (void*)111);
    int f2 = sched->CreateCoroutine(func2, (void*)222);

    while (!stop)
    {
        stop = true;
        if (sched->IsCoroutineAlive(f1))
        {
            stop = false;
            const char* y1 = (const char*)sched->ResumeCoroutine(f1, (uintptr_t)"resume func1");
            cout << "func1 yield:" << y1 << endl;
        }

        if (sched->IsCoroutineAlive(f2))
        {
            stop = false;
            const char* y2 = (const char*)sched->ResumeCoroutine(f2, (uintptr_t)"resume func2");
            cout << "func2 yield:" << y2 << endl;
        }
    }

    delete sched;
    return 0;
}

