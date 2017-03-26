#ifndef threadpool_h__
#define threadpool_h__

#include <Windows.h>
#include <vector>
#include <queue>
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

	inline void SetMaxThread(unsigned int max) { MaxThread = (max>0&&max<UINT_MAX)?max:DEFAULT_MAX_THREAD; }
	inline void SetMinThread(unsigned int min) { MinThread = (min>0&&min<UINT_MAX)?min:DEFAULT_MIN_THREAD; }
	
	void Start();
	void Stop();
	void Run(Poolable* p);

private:
	static ThreadPool* theInstance;
	unsigned int MaxThread;
	unsigned int MinThread;
	unsigned int NumOfIdleThread;
	bool bInitialized;
	bool bRunning;

	typedef std::priority_queue<Poolable*, std::vector<Poolable*>, PoolableComparision > PRIORITYQUEUE;
	PRIORITYQUEUE TaskQueue;
	CRITICAL_SECTION csTaskQueueGuard;

	typedef unsigned int THREAD_ID;
	typedef std::map<THREAD_ID, ThreadState> THREADMAP;
	typedef THREADMAP::iterator THREADMAP_ITER;
	THREADMAP ThreadMgr;

private:
	ThreadPool(unsigned int min=DEFAULT_MIN_THREAD, unsigned int max=DEFAULT_MAX_THREAD);
	ThreadPool(const ThreadPool& other);
	ThreadPool operator =(const ThreadPool& other);

	void Initialize();
	void Terminate();
	void CreateAndStartThread();
	static unsigned int __stdcall PollTask(void *p_this);
};

#endif // threadpool_h__
