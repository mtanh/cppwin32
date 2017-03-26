#include "poolable.h"

Poolable::Poolable( TaskPriority priority/*=PRIORITY_NORMAL*/, bool destroy_on_complete/*=true*/ )
: Priority(priority)
, bDestroyOnComplete(destroy_on_complete)
{
}

Poolable::~Poolable()
{
}
