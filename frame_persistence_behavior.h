#ifndef FRAME_PERSISTENCE_BEHAVIOR_H
#define FRAME_PERSISTENCE_BEHAVIOR_H

#include "frame_persistence_types.h"
#include "callback.h"

#include <functional>
#include <vector>

class FramePersistanceBehavior
{
    ICallback *m_Callback;

public:

    FramePersistanceBehavior() :
        FramePersistanceBehavior(ResponseBack())
    {

    }

    FramePersistanceBehavior(const ResponseBack &obj) {

    }

    FramePersistanceBehavior(const CollectAfterTimeout &obj) {

    }

    bool HasCallback() const {
        if(m_Callback == NULL) {
            return false;
        }
        return true;
    }

    void AddFrameReturn(int frame_id, const std::vector<uint8_t> &data) {

    }

    template <typename T>
    void setCallback(const std::function<void(const std::vector<T> &)> &callback) {

        std::shared_ptr<std::vector<T>> vec = std::make_shared<std::vector<T>>();

        m_Callback = new Callback<T>([this, callback, vec](int frame_id, const T &data) {
            vec->push_back(data);
        });
    }
};

#endif // FRAME_PERSISTENCE_BEHAVIOR_H
