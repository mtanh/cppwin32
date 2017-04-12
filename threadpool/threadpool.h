#ifndef threadpool_h__
#define threadpool_h__

#include <Windows.h>
#include <process.h>
#include <vector>
#include <queue>
#include <map>
#include <limits>
#include "poolablecomparision.h"

#define TIME_TO_WAIT 1 // 1 minute
#define WAIT_FOR_TASK (TIME_TO_WAIT*60*1000)

const int DEFAULT_MIN_THREAD = 1;
const int DEFAULT_MAX_THREAD = -1;

class Poolable;

struct ThreadState
{
	bool Working;		// worker thread is on its task?
	bool QuitAllowed;	// worker thread is idle too long (ex. 60s), should be quit?
	bool Zombie;		// thread pool is NOT running now, worker thread becomes a zombie?
	bool Finished;		// worker thread was joined, it done official and should be removed from thread map?

	ThreadState(): Working(false), QuitAllowed(false), Zombie(false), Finished(false) {}
};

class ThreadPool
{
public:
	~ThreadPool();
	
	static ThreadPool& GetInstance();

	/*inline void SetMaxThread(int max) { MaxThread = (max>0&&max<INT_MAX)?max:DEFAULT_MAX_THREAD; }
	inline void SetMinThread(int min) { MinThread = (min>0&&min<INT_MAX)?min:DEFAULT_MIN_THREAD; }*/
	
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

	HANDLE ThreadHndTbl[MAXIMUM_WAIT_OBJECTS];
	int ThreadHndCnt;

private:
	ThreadPool(int maxThread=DEFAULT_MAX_THREAD, int minThread=DEFAULT_MIN_THREAD);
	ThreadPool(const ThreadPool& other);
	ThreadPool operator =(const ThreadPool& other);

	static void Initialize();
	void Terminate();
	void CreateAndStartThread();
	void Enqueue(Poolable* p);
	Poolable* Dequeue();
	static unsigned int __stdcall PollTask(void *pThis);
	void RunImmediately(Poolable* p);
};

#endif // threadpool_h__
