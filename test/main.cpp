#include <stdio.h>
#include <assert.h>
#include "threadpool.h"
#include "logger.h"
#include "writer.h"

ThreadPool* globalThreadPool = NULL;

int main(int argc, char **argv)
{
	char* theString = "A"; // Enable ALL log bit
	unsigned int theLogMask = LogMaskStr2Mask(theString);
	if(theLogMask)
	{
		char logDir[] = "./logs/";
		CreateDirectory(logDir, NULL);
		assert(logDir[0]!=0);
		assert(logDir[strlen(logDir)-1]=='/');

		InitLogEx(logDir,
			"DIC",
			LOG_OPTION_USE_SINGLE_FILE|LOG_OPTION_SWITCH_FILES|LOG_OPTION_LOG_THREAD_ID|LOG_OPTION_AUTO_PURGE,
			DEFAULT_NKEEP);

		SetLogMask(theLogMask);
		SetAutoPurgePeriod(10); // 10 days
	}

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

	/*globalThreadPool->Run(writeFile05);
	globalThreadPool->Run(writeFile04);*/
	globalThreadPool->Run(writeFile02);
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
	globalThreadPool->Run(writeFile11);

	Sleep(200000L);
	globalThreadPool->Stop();
	globalThreadPool->Terminate();

	return 0;
}