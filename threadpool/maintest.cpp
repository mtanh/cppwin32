#include <stdio.h>
#include "threadpool.h"
#include "writefile.h"

int main(unsigned int argc, char* argv[])
{
	ThreadPool& theGlobalInstance = ThreadPool::GetInstance();
	theGlobalInstance.Start();

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

	theGlobalInstance.Run(writeFile05);
	theGlobalInstance.Run(writeFile04);
	theGlobalInstance.Run(writeFile02);
	theGlobalInstance.Run(writeFile03);
	theGlobalInstance.Run(writeFile01);
	theGlobalInstance.Run(writeFile06);
	theGlobalInstance.Run(writeFile08);
	theGlobalInstance.Run(writeFile07);
	theGlobalInstance.Run(writeFile10);
	theGlobalInstance.Run(writeFile09);
	theGlobalInstance.Run(writeFile15);
	theGlobalInstance.Run(writeFile14);
	theGlobalInstance.Run(writeFile12);
	theGlobalInstance.Run(writeFile13);
	theGlobalInstance.Run(writeFile11);

	Sleep(300000L);
	ThreadPool::GetInstance().Stop();
	ThreadPool::GetInstance().Terminate();

	return 0;
}