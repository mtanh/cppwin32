#ifndef poolable_h__
#define poolable_h__

#include <stddef.h>
#include "threadenums.h"

class Wakeable
{
public:
	virtual void Wake() = 0;
};

class Poolable
{
public:
	Poolable(TaskPriority priority=PRIORITY_NORMAL, bool destroy_on_complete=true);
	virtual ~Poolable();
	virtual void Wake() = 0;

public:
	TaskPriority Priority;
	bool bDestroyOnComplete;
};

#endif // poolable_h__
