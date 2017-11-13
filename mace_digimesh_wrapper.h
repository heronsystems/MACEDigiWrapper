#ifndef MACE_DIGIMESH_WRAPPER_H
#define MACE_DIGIMESH_WRAPPER_H

#include "macedigimeshwrapper_global.h"
#include <vector>
#include <map>
#include <functional>
#include "digi_mesh_baud_rates.h"

#include "serial_link.h"

#include "i_link_events.h"

class MACEDIGIMESHWRAPPERSHARED_EXPORT MACEDigiMeshWrapper : private ILinkEvents
{

private:

    struct Frame{
        std::function<void(const std::string &)> callback;
        bool inUse;
        bool callbackEnabled;
    };

    SerialLink *m_Link;

    std::vector<int> m_OwnVehicles;
    std::map<int, int> m_RemoteVehiclesToAddress;

    std::function<void(const int)> m_NewVehicleCallback;
    std::function<void(const std::vector<uint8_t> &)> m_NewDataCallback;

    Frame *m_CurrentFrames;
    int m_PreviousFrame;
    std::mutex m_FrameSelectionMutex;

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

    void GetParameterAsync(const std::string &parameterName, const std::function<void(const std::string &)> &callback, const std::vector<char> &data = {});

    void SetParameter(const std::string &parameterName, const std::string &value) const;

private:

    virtual void ReceiveData(SerialLink *link_ptr, const std::vector<uint8_t> &buffer) const;

    virtual void CommunicationError(const SerialLink* link_ptr, const std::string &type, const std::string &msg) const;

    virtual void CommunicationUpdate(const SerialLink *link_ptr, const std::string &name, const std::string &msg) const;

    virtual void Connected(const SerialLink* link_ptr) const;

    virtual void ConnectionRemoved(const SerialLink *link_ptr) const;

private:

    int reserve_next_frame_id();


};

#endif // MACE_DIGIMESH_WRAPPER_H
