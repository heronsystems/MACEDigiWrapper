#ifndef MACE_DIGIMESH_WRAPPER_H
#define MACE_DIGIMESH_WRAPPER_H

#include "macedigimeshwrapper_global.h"
#include <vector>
#include <map>
#include <functional>
#include "digi_mesh_baud_rates.h"

#include "serial_link.h"

#include "i_link_events.h"

#include "frame_persistence.h"

class ICallback {
public:
    virtual bool IsSet() const = 0;
};

template <typename T>
class Callback : public ICallback {
public:
    Callback() {
        m_Func = NULL;
    }

    Callback(const std::function<void(int, const T&)> &func) {
        m_Func = new std::function<void(int, const T&)>(func);
    }

    ~Callback() {
        delete m_Func;
    }

    void Call(int frame_id, const T &data) {
        (*m_Func)(frame_id, data);
    }

    virtual bool IsSet() const {
        if(m_Func == NULL) {
            return false;
        }
        return true;
    }


    std::function<void(int, const T&)> *m_Func;
};


class MACEDIGIMESHWRAPPERSHARED_EXPORT MACEDigiMeshWrapper : private ILinkEvents
{
private:

    struct Frame{
        std::shared_ptr <ICallback> callback;
        bool inUse;
    };

    SerialLink *m_Link;

    std::vector<int> m_OwnVehicles;
    std::map<int, int> m_RemoteVehiclesToAddress;

    std::function<void(const int)> m_NewVehicleCallback;
    std::function<void(const std::vector<uint8_t> &)> m_NewDataCallback;

    Frame *m_CurrentFrames;
    int m_PreviousFrame;
    std::mutex m_FrameSelectionMutex;

    std::vector<uint8_t> m_CurrBuf;

public:
    MACEDigiMeshWrapper(const std::string &commPort, const DigiMeshBaudRates &baudRate);

    ~MACEDigiMeshWrapper();

    /**
     * @brief SetOnNewVehicleCallback
     * Set lambda to be called when a new vehicle is discovered by DigiMesh
     * @param func lambda to call.
     */
    void SetOnNewVehicleCallback(std::function<void(const int)> func);


    /**
     * @brief SetNewDataCallback
     * Set callback to be notified when new data has been transmitted to this node
     * @param func Function to call upon new data
     */
    void SetNewDataCallback(std::function<void(const std::vector<uint8_t> &)> func);


    /**
     * @brief AddVehicle
     * Add a vehicle to the DigiMesh network.
     * @param ID Unique Identifier of vehicle
     */
    void AddVehicle(const int ID);


    /**
     * @brief BroadcastData
     * Broadcast data to all nodes on network
     * @param data
     */
    void BroadcastData(const std::vector<uint8_t> &data);


    /**
     * @brief SendData
     * Send data to a specific vehicle
     * @param vechileID
     * @param data
     */
    void SendData(const int vechileID, const std::vector<uint8_t> &data);


    template <typename T>
    void GetATParameterAsync(const std::string &parameterName, const std::function<void(const std::vector<T> &)> &callback, IFramePersistence &persistance)
    {
        std::shared_ptr<std::vector<T>> vec = std::make_shared<std::vector<T>>();

        std::shared_ptr<Callback<T>> cb = std::make_shared<Callback<T>>([this, callback, vec, &persistance](int frame_id, const T &data) {
            vec->push_back(data);
            persistance.FrameReturned();
        });

        int frame_id = AT_command_helper(parameterName, cb);

        persistance.AddEndEvent([this, vec, frame_id, callback](){
           finish_frame(frame_id);
           callback(*vec);
        });
    }


    void GetATParameterSync(const std::string &parameterName)
    {
        throw std::runtime_error("Not Implimented");
    }


    template <typename T>
    void SetATParameterAsync(const std::string &parameterName, const std::string &value, const std::function<void(const std::vector<T> &)> &callback = [](const std::vector<T> &){})
    {
        std::shared_ptr<Callback<T>> cb = std::make_shared<Callback<T>>([this, callback](int frame_id, const T &data) {
            finish_frame(frame_id);
            std::vector<T> vec;
            vec.push_back(data);
            callback(vec);
        });
        std::vector<char> data(value.c_str(), value.c_str() + value.length() + 1);
        AT_command_helper(parameterName, cb, data);
    }

private:

    virtual void ReceiveData(SerialLink *link_ptr, const std::vector<uint8_t> &buffer);

    virtual void CommunicationError(const SerialLink* link_ptr, const std::string &type, const std::string &msg);

    virtual void CommunicationUpdate(const SerialLink *link_ptr, const std::string &name, const std::string &msg);

    virtual void Connected(const SerialLink* link_ptr);

    virtual void ConnectionRemoved(const SerialLink *link_ptr);

private:

    int AT_command_helper(const std::string &parameterName, const std::shared_ptr<Callback<std::string>> &callback, const std::vector<char> &data = {});

    void handle_AT_command_response(const std::vector<uint8_t> &buff);

    int reserve_next_frame_id();

    template<typename T>
    void find_and_invokve_frame(int frame_id, const T &data)
    {
        if(this->m_CurrentFrames[frame_id].callback != NULL && this->m_CurrentFrames[frame_id].callback->IsSet() == true) {
            ((Callback<T>*)this->m_CurrentFrames[frame_id].callback.get())->Call(frame_id, data);
        }
    }

    void finish_frame(int frame_id);


};

#endif // MACE_DIGIMESH_WRAPPER_H
