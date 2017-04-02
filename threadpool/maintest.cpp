#include <stdio.h>
#include "threadpool.h"

int main(unsigned int argc, char* argv[])
{
	ThreadPool::GetInstance().Start();

	for(;;)
	{
		Sleep(500);
	}

	return 0;
}