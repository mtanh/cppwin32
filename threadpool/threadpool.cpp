
#include "threadpool.h"

ThreadPool* ThreadPool::theInstance = NULL;

ThreadPool::ThreadPool( unsigned int min/*=DEFAULT_MIN_THREAD*/, unsigned int max/*=DEFAULT_MAX_THREAD*/ )
: MinThread(min)
, MaxThread(max)
, NumOfIdleThread(0)
, NumOfCurrentThread(0)
, bRunning(false)
{
	InitializeCriticalSection(&csTaskQueueGuard);
	InitializeCriticalSection(&csThreadMgrGuard);
}

ThreadPool::~ThreadPool()
{
	Terminate();
}

void ThreadPool::Initialize()
{
	theInstance = new ThreadPool();
}

void ThreadPool::Terminate()
{
	DeleteCriticalSection(&csTaskQueueGuard);
	DeleteCriticalSection(&csThreadMgrGuard);

	if(theInstance)
	{
		delete theInstance;
	}
	theInstance = NULL;
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
	if(!bRunning)
	{
		// first of all, need to enable bRunning so that the worker threads can work
		bRunning = true;

		for(int i=0; i<MinThread; ++i)
		{
			CreateAndStartThread();
		}
	}
}

void ThreadPool::Stop()
{
	if(bRunning)
	{
		bRunning = false;

		// signal all worker thread that is sleeping
	}
}

void ThreadPool::CreateAndStartThread()
{
	THREAD_ID threadId;
	HANDLE hThreadId = (HANDLE)_beginthreadex(
		NULL, // security
		0, // stack size or 0
		PollTask, 
		this,
		0, // initial state
		&threadId);

	ThreadState ts;
	EnterCriticalSection(&csThreadMgrGuard);
	ThreadMgr.insert(std::make_pair(threadId, ts));
	NumOfIdleThread++;
	NumOfCurrentThread++;
	LeaveCriticalSection(&csThreadMgrGuard);
}

unsigned int __stdcall ThreadPool::PollTask( void *pThis )
{
	ThreadPool* tp = (ThreadPool*)pThis;
	if(tp)
	{
		while(tp->bRunning)
		{
			Sleep(500);
		}
	}
	return 1;
}

void ThreadPool::Enqueue( Poolable* p )
{
	EnterCriticalSection(&csTaskQueueGuard);
	TaskQueue.push(p);
	LeaveCriticalSection(&csTaskQueueGuard);
}

Poolable* ThreadPool::Dequeue()
{
	return NULL;
}
