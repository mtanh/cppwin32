#include <time.h>
#include <assert.h>
#include "poolable.h"
#include "threadpool.h"

ThreadPool* ThreadPool::theInstance = NULL;

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
	InitializeCriticalSection(&csThreadPoolState);
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

void ThreadPool::Initialize()
{
	theInstance = new ThreadPool();
}

void ThreadPool::Terminate()
{
	DeleteCriticalSection(&csTaskQueueGuard);
	DeleteCriticalSection(&csThreadMgrGuard);
	DeleteCriticalSection(&csThreadPoolState);

	// close worker event/thread handles
	for(int i=0; i<ThreadHndCnt; ++i)
	{
		CloseHandle(ThreadHndTbl[i]);
	}

	//CloseHandle(ThreadCheckerHnd);
	//CloseHandle(TimedOutEvent);

	ThreadMgr.clear(); // clear the thread manager

	if(theInstance)
	{
		delete theInstance;
	}
	theInstance = NULL;
	printf("All resources was destroyed\n");
}

ThreadPool& ThreadPool::GetInstance()
{
	if(NULL == theInstance)
	{
		Initialize();
	}
	return *theInstance;
}

void ThreadPool::Run( Poolable* p )
{
	if(p != NULL)
	{
		EnterCriticalSection(&csThreadMgrGuard);
		//printf("NumOfIdleThread: %d\n", NumOfIdleThread);
		//printf("NumOfCurrentThread: %d\n", NumOfCurrentThread);
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
	EnterCriticalSection(&csThreadPoolState);
	if(!bRunning)
	{
		// first of all, need to enable bRunning so that the worker threads can work
		bRunning = true;
		LeaveCriticalSection(&csThreadPoolState);

		for(int i=0; i<MinThread; ++i)
		{
			CreateAndStartThread();
		}

		// start the monitoring thread
		//THREAD_ID threadId;
		//ThreadCheckerHnd = (HANDLE)_beginthreadex(
		//	NULL, // security
		//	0, // stack size or 0
		//	ResourceCheckingTimer, 
		//	this,
		//	0, // initial state
		//	&threadId);

		// create event to control the monitoring thread
		//TimedOutEvent = CreateEvent(NULL, // default security attributes
		//	TRUE, // manual-reset event object
		//	FALSE, // initial state is non-signaled
		//	NULL);
	}
	LeaveCriticalSection(&csThreadPoolState);
}

void ThreadPool::Stop()
{
	EnterCriticalSection(&csThreadPoolState);
	if(bRunning)
	{
		bRunning = false;
		LeaveCriticalSection(&csThreadPoolState);

		// signal all worker thread that is sleeping and wait for all worker thread done
		WakeAllConditionVariable(&cvTaskQueueIsNotEmpty);
		WaitForMultipleObjects(ThreadHndCnt, ThreadHndTbl,
			TRUE, // wait for all done
			INFINITE); // wait timeout

		// wait for monitoring thread done
		/*SetEvent(TimedOutEvent);
		WaitForSingleObject(ThreadCheckerHnd, INFINITE);*/
	}
	LeaveCriticalSection(&csThreadPoolState);
}

void ThreadPool::CreateAndStartThread()
{
	THREAD_ID threadId;
	HANDLE threadHandle = (HANDLE)_beginthreadex(
		NULL, // security
		0, // stack size or 0
		PollTask, 
		this,
		0, // initial state
		&threadId);

	ThreadState ts;
	EnterCriticalSection(&csThreadMgrGuard);
	ThreadMgr.insert(std::make_pair(threadId, ts));
	char buf[1024] = {0};
	sprintf(buf, "Thread %d added\n", threadId);
	printf(buf);

	ThreadHndTbl[ThreadHndCnt++] = threadHandle;
	//NumOfIdleThread++;
	NumOfCurrentThread++;
	LeaveCriticalSection(&csThreadMgrGuard);
}

unsigned int __stdcall ThreadPool::PollTask( void *pThis )
{
	ThreadPool* tp = (ThreadPool*)pThis;
	if(NULL == tp)
	{
		return 0;
	}

	THREAD_ID threadId = (THREAD_ID)GetCurrentThreadId();
	EnterCriticalSection(&tp->csThreadMgrGuard);
	THREADMAP_ITER iter = tp->ThreadMgr.find(threadId);
	if(iter == tp->ThreadMgr.end())
	{
		LeaveCriticalSection(&tp->csThreadMgrGuard);
		return 0;
	}
	LeaveCriticalSection(&tp->csThreadMgrGuard);
	ThreadState &ts = iter->second;

	while(true)
	{
		EnterCriticalSection(&tp->csThreadPoolState);
		if(tp->bRunning)
		{
			LeaveCriticalSection(&tp->csThreadPoolState);

			/*EnterCriticalSection(&tp->csThreadMgrGuard);
			if(ts.QuitAllowed)
			{
				char buf[1024] = {0};
				sprintf(buf, "Thread %d has no longer task. should be quit\n", threadId);
				printf(buf);

				LeaveCriticalSection(&tp->csThreadMgrGuard);
				break;
			}
			LeaveCriticalSection(&tp->csThreadMgrGuard);*/

			Poolable* taskItem = tp->Dequeue(); // block here
			if(NULL == taskItem)
			{
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
			LeaveCriticalSection(&tp->csThreadPoolState);
			break;
		}
	}

	// the worker thread no longer works 
	EnterCriticalSection(&tp->csThreadMgrGuard);
	tp->NumOfCurrentThread--;

	tp->ThreadMgr.erase(threadId);
	char buf[1024] = {0};
	sprintf(buf, "Removed %d from map. New size: %d\n", threadId, tp->ThreadMgr.size());
	printf(buf);
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
	if(NULL == tp)
	{
		return 0;
	}

	while(true)
	{
		EnterCriticalSection(&tp->csThreadPoolState);
		if(tp->bRunning)
		{
			LeaveCriticalSection(&tp->csThreadPoolState);
			DWORD dwWaitResult = WaitForSingleObject(tp->TimedOutEvent, WAIT_FOR_CHECK); // 5 seconds interval
			switch(dwWaitResult)
			{
				// TimedOutEvent is signaled
				case WAIT_OBJECT_0+0:
					break;

				case WAIT_TIMEOUT:
					{
						char buf[1024] = {0};
						sprintf(buf, "Timed out. Start check: %lu\n", time(0L));
						printf(buf);

						tp->CheckAndReleasePoolResource();
						break;
					}

				default:
					break;
			}
		}
		else
		{
			LeaveCriticalSection(&tp->csThreadPoolState);
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
