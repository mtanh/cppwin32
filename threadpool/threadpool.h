#ifndef threadpool_h__
#define threadpool_h__

#include <Windows.h>
#include <process.h>
#include <vector>
#include <queue>
#include <map>
#include <limits>
#include <time.h>
#include "poolablecomparision.h"

#define WAIT_FOR_TASK 5000L // 5 seconds
#define WAIT_FOR_CHECK 60000L // 60 seconds

const int DEFAULT_MIN_THREAD = 1;
const int DEFAULT_MAX_THREAD = -1;

class Poolable;

struct ThreadState
{
	bool Working;		// worker thread is on its task
	bool QuitAllowed;	// worker thread is idle too long (ex. 60s), should be quit
	time_t IdleTimeStamp;

	ThreadState(): Working(false), QuitAllowed(false), IdleTimeStamp(time(0L)) {}
};

class ThreadPool
{
public:
	~ThreadPool();
	
	static ThreadPool& GetInstance();
	void Start(); // start the pool looping
	void Stop(); // stop the pool, may be started later
	void Run(Poolable* p);

	// terminate the pool looping, should destroy all pool's resource here
	// use with care, the Stop function should be called before it
	void Terminate(); 

private:
	static ThreadPool* theInstance;
	int MaxThread;
	int MinThread;
	int NumOfIdleThread;
	int NumOfCurrentThread;
	bool bRunning;
	CRITICAL_SECTION csThreadPoolState;

	typedef std::priority_queue<Poolable*, std::vector<Poolable*>, PoolableComparision > PRIORITYQUEUE;
	PRIORITYQUEUE TaskQueue;
	CRITICAL_SECTION csTaskQueueGuard;
	CONDITION_VARIABLE cvTaskQueueIsNotEmpty;

	typedef unsigned int THREAD_ID;
	typedef std::map<THREAD_ID, ThreadState> THREADMAP;
	typedef THREADMAP::iterator THREADMAP_ITER;
	THREADMAP ThreadMgr;
	CRITICAL_SECTION csThreadMgrGuard;

	// manage worker threads
	HANDLE ThreadHndTbl[MAXIMUM_WAIT_OBJECTS];
	int ThreadHndCnt;

	// map the ThreadId and ThreadHandle
	/*typedef std::map<THREAD_ID, HANDLE> THREADIDMAP;
	typedef std::map<THREAD_ID, HANDLE>::iterator THREADIDMAP_ITER;
	THREADIDMAP ThreadIdMapper;*/

	// the thread that manage and release the worker thread resources
	HANDLE ThreadCheckerHnd;
	HANDLE TimedOutEvent;
	
private:
	ThreadPool(int maxThread=DEFAULT_MAX_THREAD, int minThread=DEFAULT_MIN_THREAD);
	ThreadPool(const ThreadPool& other);
	ThreadPool operator =(const ThreadPool& other);

	static void Initialize();
	void CreateAndStartThread();
	void Enqueue(Poolable* p);
	Poolable* Dequeue();
	void CheckAndReleasePoolResource();
	static unsigned int __stdcall PollTask(void* pThis);
	static unsigned int __stdcall ResourceCheckingTimer(void* pThis);
	void RunImmediately(Poolable* p);
};

#endif // threadpool_h__
