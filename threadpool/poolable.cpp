#include "poolable.h"

Poolable::Poolable( TaskPriority priority/*=PRIORITY_NORMAL*/, bool destroyOnComplete/*=true*/ )
: Priority(priority)
, bDestroyOnComplete(destroyOnComplete)
{
}

Poolable::~Poolable()
{
}
