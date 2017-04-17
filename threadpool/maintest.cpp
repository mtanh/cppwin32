#include <stdio.h>
#include "threadpool.h"
#include "writefile.h"

ThreadPool* globalThreadPool;

int main(unsigned int argc, char* argv[])
{
	ThreadPool tp;
	globalThreadPool = &tp;
	//ThreadPool& theGlobalInstance = ThreadPool::GetInstance();
	globalThreadPool->Start();

	DicWriteFile* writeFile01 = new DicWriteFile(PRIORITY_HIGHEST, true);
	DicWriteFile* writeFile02 = new DicWriteFile(PRIORITY_HIGH, true);
	DicWriteFile* writeFile03 = new DicWriteFile();
	DicWriteFile* writeFile04 = new DicWriteFile(PRIORITY_LOW, true);
	DicWriteFile* writeFile05 = new DicWriteFile(PRIORITY_LOWEST, true);

	DicWriteFile* writeFile06 = new DicWriteFile(PRIORITY_HIGHEST, true);
	DicWriteFile* writeFile07 = new DicWriteFile(PRIORITY_HIGH, true);
	DicWriteFile* writeFile08 = new DicWriteFile();
	DicWriteFile* writeFile09 = new DicWriteFile(PRIORITY_LOW, true);
	DicWriteFile* writeFile10 = new DicWriteFile(PRIORITY_LOWEST, true);

	DicWriteFile* writeFile11 = new DicWriteFile(PRIORITY_HIGHEST, true);
	DicWriteFile* writeFile12 = new DicWriteFile(PRIORITY_HIGH, true);
	DicWriteFile* writeFile13 = new DicWriteFile();
	DicWriteFile* writeFile14 = new DicWriteFile(PRIORITY_LOW, true);
	DicWriteFile* writeFile15 = new DicWriteFile(PRIORITY_LOWEST, true);

	globalThreadPool->Run(writeFile05);
	globalThreadPool->Run(writeFile04);
	/*globalThreadPool->Run(writeFile02);
	globalThreadPool->Run(writeFile03);
	globalThreadPool->Run(writeFile01);
	globalThreadPool->Run(writeFile06);
	globalThreadPool->Run(writeFile08);
	globalThreadPool->Run(writeFile07);
	globalThreadPool->Run(writeFile10);
	globalThreadPool->Run(writeFile09);
	globalThreadPool->Run(writeFile15);
	globalThreadPool->Run(writeFile14);
	globalThreadPool->Run(writeFile12);
	globalThreadPool->Run(writeFile13);
	globalThreadPool->Run(writeFile11);*/

	Sleep(200000L);
	globalThreadPool->Stop();
	globalThreadPool->Terminate();

	return 0;
}