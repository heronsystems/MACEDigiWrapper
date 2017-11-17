#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include <functional>
#include <memory>

#include "../types/shutdown-first-response.h"

template <typename ...T>
class FramePersistanceBehavior;


template <>
class FramePersistanceBehavior<>
{
    std::shared_ptr<std::function<void()>> m_SendFramesUp;
    std::shared_ptr<std::function<void(const std::vector<uint8_t> &data)>> m_NewFrame;

public:

    FramePersistanceBehavior() :
        m_NewFrame(NULL),
        m_SendFramesUp(NULL)
    {
    }



    bool HasCallback() const {
        if(m_SendFramesUp == NULL) {
            return false;
        }
        return true;
    }

    void AddFrameReturn(int frame_id, const std::vector<uint8_t> &data) {
        (*m_NewFrame)(data);
    }

    void SendAndFinish() {
        (*m_SendFramesUp)();
        m_SendFramesUp = NULL;
        m_NewFrame = NULL;
    }

    template <typename T>
    void setCallback(const std::function<void(const std::vector<T> &)> &callback) {
        std::shared_ptr<std::vector<T>> vec = std::make_shared<std::vector<T>>();

        m_NewFrame = std::make_shared<std::function<void(const std::vector<uint8_t> &data)>>([this, vec](const std::vector<uint8_t> &data)
        {
            vec->push_back(T(data));
            FrameReceived();
        });

        m_SendFramesUp = std::make_shared<std::function<void()>>([callback, vec](){
            callback(*vec);
        });
    }

    virtual void FrameReceived() = 0;
};

#endif // BEHAVIOR_H
