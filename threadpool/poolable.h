#ifndef poolable_h__
#define poolable_h__

#include <stddef.h>
#include "threadenums.h"

class Poolable
{
public:
	Poolable(TaskPriority priority=PRIORITY_NORMAL, bool destroy_on_complete=true);
	virtual ~Poolable();

	virtual void RunImmediately(void* arg=NULL) = 0;
	//friend bool operator >(Poolable& p1, Poolable& p2);
	//friend bool operator <(Poolable& p1, Poolable& p2);

public:
	TaskPriority Priority;
	bool bDestroyOnComplete;

//private:
//	Poolable(const Poolable& other);
//	Poolable operator =(const Poolable& other);
};

#endif // poolable_h__
