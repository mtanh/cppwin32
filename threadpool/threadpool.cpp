#include "threadpool.h"

ThreadPool* ThreadPool::theInstance = NULL;

ThreadPool::ThreadPool( unsigned int min/*=DEFAULT_MIN_THREAD*/, unsigned int max/*=DEFAULT_MAX_THREAD*/ )
: MinThread(min)
, MaxThread(max)
, NumOfIdleThread(0)
, bInitialized(false)
, bRunning(false)
{
}

ThreadPool::~ThreadPool()
{

}

void ThreadPool::Initialize()
{
	theInstance = new ThreadPool();

	InitializeCriticalSection(&csTaskQueueGuard);

	bInitialized = true;
}

void ThreadPool::Terminate()
{
	DeleteCriticalSection(&csTaskQueueGuard);

	if(theInstance)
	{
		delete theInstance;
	}
	theInstance = NULL;
}

ThreadPool& ThreadPool::GetInstance()
{
	if(!bInitialized)
	{
		Initialize();
	}
	return *theInstance;
}

void ThreadPool::Run( Poolable* p )
{

}

void ThreadPool::Start()
{
	if(!bRunning)
	{
		// first of all, need to enable bRunning so that the threads can work
		bRunning = true;

		for(int i=0; i<MinThread; ++i)
		{
			CreateAndStartThread();
		}
	}
}

void ThreadPool::Stop()
{

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


}

unsigned int __stdcall ThreadPool::PollTask( void *p_this )
{

}
