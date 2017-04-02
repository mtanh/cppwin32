#ifndef threadpool_h__
#define threadpool_h__

#include <Windows.h>
#include <process.h>
#include <vector>
#include <queue>
#include <map>
#include <limits>

#include "poolablecomparision.h"

const unsigned int DEFAULT_MIN_THREAD = 1;
const unsigned int DEFAULT_CPU_CORE = 4;
const unsigned int DEFAULT_MAX_THREAD = (DEFAULT_CPU_CORE*2+1);

class Poolable;

struct ThreadState
{
	bool Working;
	bool QuitAllowed;
	bool Finished;

	ThreadState(): Working(false), QuitAllowed(false), Finished(false) {}
};

class ThreadPool
{
public:
	~ThreadPool();
	
	static ThreadPool& GetInstance();

	inline void SetMaxThread(int max) { MaxThread = (max>0&&max<INT_MAX)?max:DEFAULT_MAX_THREAD; }
	inline void SetMinThread(int min) { MinThread = (min>0&&min<INT_MAX)?min:DEFAULT_MIN_THREAD; }
	
	void Start();
	void Stop();
	void Run(Poolable* p);

private:
	static ThreadPool* theInstance;
	int MaxThread;
	int MinThread;
	int NumOfIdleThread;
	int NumOfCurrentThread;
	bool bRunning;

	typedef std::priority_queue<Poolable*, std::vector<Poolable*>, PoolableComparision > PRIORITYQUEUE;
	PRIORITYQUEUE TaskQueue;
	CRITICAL_SECTION csTaskQueueGuard;

	typedef unsigned int THREAD_ID;
	typedef std::map<THREAD_ID, ThreadState> THREADMAP;
	typedef THREADMAP::iterator THREADMAP_ITER;
	THREADMAP ThreadMgr;
	CRITICAL_SECTION csThreadMgrGuard;

private:
	ThreadPool(unsigned int min=DEFAULT_MIN_THREAD, unsigned int max=DEFAULT_MAX_THREAD);
	ThreadPool(const ThreadPool& other);
	ThreadPool operator =(const ThreadPool& other);

	static void Initialize();
	void Terminate();
	void CreateAndStartThread();
	void Enqueue(Poolable* p);
	Poolable* Dequeue();
	static unsigned int __stdcall PollTask(void *pThis);
};

#endif // threadpool_h__
