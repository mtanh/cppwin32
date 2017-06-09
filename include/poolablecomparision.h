#ifndef poolablecomparision_h__
#define poolablecomparision_h__

#include "threadenums.h"

class Poolable;

class PoolableComparision
{
public:
	bool operator()(Poolable* p1, Poolable* p2) const;

private:
	bool LessThan(TaskPriority tp1, TaskPriority tp2) const { return ((int)tp1<(int)tp2); }
};

#endif // poolablecomparision_h__
