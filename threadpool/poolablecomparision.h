#ifndef poolablecomparision_h__
#define poolablecomparision_h__

#include "threadenums.h"

class Poolable;

class PoolableComparision
{
public:
	bool operator()(Poolable* p1, Poolable* p2) const;

private:
	bool GreatThan(TaskPriority tp1, TaskPriority tp2) const { return (tp1>tp2); }
};

#endif // poolablecomparision_h__
