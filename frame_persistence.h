#ifndef FRAMEPERSISTENCE_H
#define FRAMEPERSISTENCE_H

#include <functional>
#include <vector>

#include "timer.h"

class IFramePersistence {
private:
    std::vector<std::function<void()>> m_EndEvent;
    int m_FrameID;

public:
    AddEndEvent(const std::function<void()> &func) {
        m_EndEvent.push_back(func);
    }

    virtual void FrameReturned() = 0;

    Trigger() {
        for(size_t i = 0 ; i < m_EndEvent.size() ; i++) {
            m_EndEvent.at(i)();
        }
        m_EndEvent.clear();
    }

    SetFrameID(int frame_id) {
        m_FrameID = frame_id;
    }

    GetFrameID() const {
        return m_FrameID;
    }
};

class ResponseBack : public IFramePersistence
{
    virtual void FrameReturned()
    {
        Trigger();
    }
};

class CollectAfterTimeout : public IFramePersistence
{
private:

    int m_NumMS;
    Timer *m_Timer;

public:
    CollectAfterTimeout(int numberMS) :
        m_NumMS(numberMS),
        m_Timer(NULL)
    {
        m_Timer = new Timer(numberMS, [this](){
            this->Trigger();
        });
    }

    ~CollectAfterTimeout() {
        delete m_Timer;
    }

    virtual void FrameReturned() {

    }


};

#endif // FRAMEPERSISTENCE_H
