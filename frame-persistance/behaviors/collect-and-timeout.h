#ifndef COLLECT_AND_TIMEOUT_H
#define COLLECT_AND_TIMEOUT_H

#include "base.h"
#include "../types/collect-and-timeout.h"




template <typename ...Rest>
class FramePersistanceBehavior<CollectAfterTimeout, Rest...> : public FramePersistanceBehavior<Rest...>
{
    static_assert(sizeof...(Rest) == 0, "Only one argument can be passed");
public:

    FramePersistanceBehavior(const CollectAfterTimeout & params) {

    }
};

#endif // COLLECT_AND_TIMEOUT_H
