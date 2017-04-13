#include <stdio.h>
#include "threadpool.h"

int main(unsigned int argc, char* argv[])
{
	ThreadPool::GetInstance().Start();
	Sleep(188000L);
	ThreadPool::GetInstance().Stop();
	ThreadPool::GetInstance().Terminate();

	return 0;
}