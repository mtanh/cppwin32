#ifndef writefile_h__
#define writefile_h__

#include "poolable.h"

class DicWriteFile: public Poolable
{
public:
	DicWriteFile();
	DicWriteFile(TaskPriority priority, bool destroy_on_complete);
	~DicWriteFile();

	/*virtual*/ void Wake();
};

DicWriteFile::DicWriteFile()
: Poolable()
{
}

DicWriteFile::DicWriteFile( TaskPriority priority, bool destroy_on_complete )
:Poolable(priority, destroy_on_complete)
{
}

DicWriteFile::~DicWriteFile()
{
}

void DicWriteFile::Wake()
{
	Sleep(15000);
}

#endif // writefile_h__
