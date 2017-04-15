#include "poolablecomparision.h"
#include "poolable.h"

bool PoolableComparision::operator()( Poolable* p1, Poolable* p2 ) const
{
	return LessThan(p1->Priority, p2->Priority);
}
