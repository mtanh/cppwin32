#include <stdio.h>
#include "threadpool.h"

int main(unsigned int argc, char* argv[])
{
	ThreadPool::GetInstance().Start();
	Sleep(120000);
	ThreadPool::GetInstance().Stop();

	return 0;
}