#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include "../../callback.h"

#include "../types/shutdown-first-response.h"

template <typename ...T>
class FramePersistanceBehavior;


template <>
class FramePersistanceBehavior<>
{
    ICallback *m_Callback;

public:


    bool HasCallback() const {
        if(m_Callback == NULL) {
            return false;
        }
        return true;
    }

    void AddFrameReturn(int frame_id, const std::vector<uint8_t> &data) {
        m_Callback->Call(frame_id, data);
    }

    template <typename T>
    void setCallback(const std::function<void(const std::vector<T> &)> &callback) {

        std::shared_ptr<std::vector<T>> vec = std::make_shared<std::vector<T>>();

        m_Callback = new Callback<T>([this, callback, vec](int frame_id, const T &data) {
            vec->push_back(data);
        });
    }
};

#endif // BEHAVIOR_H
