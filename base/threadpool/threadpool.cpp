#include <time.h>
#include <assert.h>
#include <algorithm>
#include "poolable.h"
#include "threadpool.h"

//ThreadPool* ThreadPool::theInstance = NULL;

ThreadPool::ThreadPool( int maxThread/*=DEFAULT_MAX_THREAD*/, int minThread/*=DEFAULT_MIN_THREAD*/ )
: MinThread(minThread)
, MaxThread(maxThread)
, NumOfIdleThread(0)
, NumOfCurrentThread(0)
, ThreadHndCnt(0)
, bRunning(false)
{
	InitializeCriticalSection(&csTaskQueueGuard);
	InitializeCriticalSection(&csThreadMgrGuard);
	InitializeCriticalSection(&csThreadPoolStateGuard);
	InitializeConditionVariable(&cvTaskQueueIsNotEmpty);

	if(maxThread < 0)
	{
		SYSTEM_INFO sysinfo;
		::GetSystemInfo(&sysinfo);
		MaxThread = ((int)sysinfo.dwNumberOfProcessors)/**2+1*/;
	}
}

ThreadPool::~ThreadPool()
{
}

//void ThreadPool::Initialize()
//{
//	theInstance = new ThreadPool();
//}

void ThreadPool::Terminate()
{
	EnterCriticalSection(&csThreadPoolStateGuard);
	if(!bRunning) // only destroy resource once Stop function was invoked
	{
		DeleteCriticalSection(&csTaskQueueGuard);
		DeleteCriticalSection(&csThreadMgrGuard);

		// close worker event/thread handles
		for(int i=0; i<ThreadHndCnt; ++i)
		{
			if(ThreadHndTbl[i]) {
				CloseHandle(ThreadHndTbl[i]);
			}
		}

		CloseHandle(ThreadCheckerHnd);
		CloseHandle(TimedOutEvent);

		ThreadMgr.clear(); // clear the thread manager

		LeaveCriticalSection(&csThreadPoolStateGuard);
		DeleteCriticalSection(&csThreadPoolStateGuard);
	}
	else {
		LeaveCriticalSection(&csThreadPoolStateGuard);
	}
}

//ThreadPool& ThreadPool::GetInstance()
//{
//	if(NULL == theInstance)
//	{
//		Initialize();
//	}
//	return *theInstance;
//}

void ThreadPool::Run( Poolable* p )
{
	if(p != NULL)
	{
		EnterCriticalSection(&csThreadMgrGuard);
		/*printf("NumOfIdleThread: %d\n", NumOfIdleThread);
		printf("NumOfCurrentThread: %d\n", NumOfCurrentThread);*/
		if(NumOfIdleThread == 0 && NumOfCurrentThread < MaxThread)
		{
			CreateAndStartThread();
		}
		LeaveCriticalSection(&csThreadMgrGuard);
		Enqueue(p);
	}
}

void ThreadPool::Start()
{
	EnterCriticalSection(&csThreadPoolStateGuard);
	if(!bRunning)
	{
		// first of all, need to enable bRunning so that the worker threads can work
		bRunning = true;
		LeaveCriticalSection(&csThreadPoolStateGuard);

		for(int i=0; i<MinThread; ++i) {
			CreateAndStartThread();
		}

		// start the monitoring thread
		THREAD_ID threadId;
		ThreadCheckerHnd = (HANDLE)_beginthreadex(
			NULL, // security
			0, // stack size or 0
			ResourceCheckingTimer, 
			this,
			0, // initial state
			&threadId);

		// create event to control the monitoring thread
		TimedOutEvent = CreateEvent(NULL, // default security attributes
			TRUE, // manual-reset event object
			FALSE, // initial state is non-signaled
			NULL);
	}
	else {
		LeaveCriticalSection(&csThreadPoolStateGuard);
	}
}

void ThreadPool::Stop()
{
	EnterCriticalSection(&csThreadPoolStateGuard);
	if(bRunning)
	{
		bRunning = false;
		LeaveCriticalSection(&csThreadPoolStateGuard);

		// signal all worker thread that is sleeping and wait for all worker thread done
		WakeAllConditionVariable(&cvTaskQueueIsNotEmpty);
		WaitForMultipleObjects(ThreadHndCnt, ThreadHndTbl,
			TRUE, // wait for all done
			INFINITE); // wait timeout

		// wait for monitoring thread done
		/*SetEvent(TimedOutEvent);
		WaitForSingleObject(ThreadCheckerHnd, INFINITE);*/
	}
	else {
		LeaveCriticalSection(&csThreadPoolStateGuard);
	}
}

void ThreadPool::CreateAndStartThread()
{
	THREAD_ID threadId;
	HANDLE threadHandle = (HANDLE)_beginthreadex(
		NULL, // security
		0, // stack size or 0
		PollTask, 
		this,
		CREATE_SUSPENDED, // initial state
		&threadId);

	EnterCriticalSection(&csThreadMgrGuard);
	ThreadState ts;
	ThreadMgr.insert(std::make_pair(threadId, ts));
	ThreadHndTbl[ThreadHndCnt++] = threadHandle;
	NumOfIdleThread++;
	NumOfCurrentThread++;
	(void)ResumeThread(threadHandle);
	LeaveCriticalSection(&csThreadMgrGuard);
}

unsigned int __stdcall ThreadPool::PollTask( void *pThis )
{
	ThreadPool* tp = (ThreadPool*)pThis;
	if(NULL == tp) {
		return 0;
	}

	THREAD_ID threadId = (THREAD_ID)GetCurrentThreadId();
	EnterCriticalSection(&tp->csThreadMgrGuard);
	THREADMAP_ITER iter = tp->ThreadMgr.find(threadId);
	/*THREADMAP_ITER iter = std::lower_bound(tp->ThreadMgr.begin(),
		tp->ThreadMgr.end(),
		threadId,
		ThreadDataCompare());*/

	if(iter == tp->ThreadMgr.end() /*|| iter->first != threadId*/)
	{
		LeaveCriticalSection(&tp->csThreadMgrGuard);
		return 0;
	}
	LeaveCriticalSection(&tp->csThreadMgrGuard);

	printf("Found thread %d in map\n", threadId);
	ThreadState &ts = iter->second;

	while(true)
	{
		EnterCriticalSection(&tp->csThreadPoolStateGuard);
		if(tp->bRunning)
		{
			LeaveCriticalSection(&tp->csThreadPoolStateGuard);

			EnterCriticalSection(&tp->csThreadMgrGuard);
			if(ts.QuitAllowed)
			{
				printf("Thread %d has no longer task. should be quit\n", threadId);
				LeaveCriticalSection(&tp->csThreadMgrGuard);
				break;
			}
			LeaveCriticalSection(&tp->csThreadMgrGuard);

			Poolable* taskItem = tp->Dequeue(); // block here
			if(NULL == taskItem) {
				continue;
			}

			// update the thread state before executing the job
			EnterCriticalSection(&tp->csThreadMgrGuard);
			tp->NumOfIdleThread--;
			ts.Working = true;
			LeaveCriticalSection(&tp->csThreadMgrGuard);

			// execute the job
			tp->RunImmediately(taskItem);

			// update the thread state after executing the job
			EnterCriticalSection(&tp->csThreadMgrGuard);
			tp->NumOfIdleThread++;
			ts.Working = false;
			time(&ts.IdleTimeStamp);
			LeaveCriticalSection(&tp->csThreadMgrGuard);
		}
		else
		{
			LeaveCriticalSection(&tp->csThreadPoolStateGuard);
			break;
		}
	}

	// the worker thread no longer works 
	EnterCriticalSection(&tp->csThreadMgrGuard);
	tp->ThreadMgr.erase(threadId);
	tp->NumOfCurrentThread--;
	tp->NumOfIdleThread--;

	printf("Removed %d from map. New size: %d\n", threadId, tp->ThreadMgr.size());

	LeaveCriticalSection(&tp->csThreadMgrGuard);

	return 1;
}

void ThreadPool::Enqueue( Poolable* p )
{
	EnterCriticalSection(&csTaskQueueGuard);
	TaskQueue.push(p);
	WakeConditionVariable(&cvTaskQueueIsNotEmpty);
	LeaveCriticalSection(&csTaskQueueGuard);
	//WakeConditionVariable(&cvTaskQueueIsNotEmpty);
}

Poolable* ThreadPool::Dequeue()
{
	EnterCriticalSection(&csTaskQueueGuard);
	while(TaskQueue.empty())
	{
		DWORD dwWaitResult = SleepConditionVariableCS(&cvTaskQueueIsNotEmpty,
			&csTaskQueueGuard, WAIT_FOR_TASK);

		// double check for fake signals
		if(TaskQueue.empty())
		{
			LeaveCriticalSection(&csTaskQueueGuard);
			return NULL;
		}
	}

	Poolable* taskItem = TaskQueue.top();
	TaskQueue.pop();
	LeaveCriticalSection(&csTaskQueueGuard);

	return taskItem;
}

void ThreadPool::RunImmediately( Poolable* p )
{
	assert(p != NULL);
	p->Wake();
	printf("[%x - %d] Done: %lu\n", p, p->Priority, time(0L));

	if(p->bDestroyOnComplete)
	{
		delete p;
		p = NULL;
	}
}

unsigned int __stdcall ThreadPool::ResourceCheckingTimer( void* pThis )
{
	ThreadPool* tp = (ThreadPool*)pThis;
	if(NULL == tp) {
		return 0;
	}

	while(true)
	{
		EnterCriticalSection(&tp->csThreadPoolStateGuard);
		if(tp->bRunning)
		{
			LeaveCriticalSection(&tp->csThreadPoolStateGuard);
			DWORD dwWaitResult = WaitForSingleObject(tp->TimedOutEvent, WAIT_FOR_CHECK); // 5 seconds interval
			switch(dwWaitResult)
			{
				// TimedOutEvent is signaled
			case WAIT_OBJECT_0+0:
				break;

			case WAIT_TIMEOUT:
				{
					/*char buf[1024] = {0};
					sprintf(buf, "Timed out. Start check: %lu\n", time(0L));
					printf(buf);*/

					tp->CheckAndReleasePoolResource();
					break;
				}

			default:
				break;
			}
		}
		else
		{
			LeaveCriticalSection(&tp->csThreadPoolStateGuard);
			break;
		}
	}
}

void ThreadPool::CheckAndReleasePoolResource()
{
	EnterCriticalSection(&csThreadMgrGuard);
	THREADMAP_ITER iter = ThreadMgr.begin();
	for(; iter != ThreadMgr.end(); ++iter)
	{
		ThreadState &threadState = iter->second;
		__time64_t idleTime = time(0L) - threadState.IdleTimeStamp;
		if(idleTime >= THREAD_IDLE_TIME_LIMIT)
		{
			// worker thread is idle/do-task too long
			threadState.QuitAllowed = true;
		}
	}
	LeaveCriticalSection(&csThreadMgrGuard);
}
