#ifndef MACE_WRAPPER_H
#define MACE_WRAPPER_H

#include "macewrapper_global.h"

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <mutex>
#include <thread>

#include "digi_mesh_baud_rates.h"
#include "transmit_status_types.h"

/**
 * @brief The MACEDigiMeshWrapper class
 *
 * This object employs its own protocol internal to itself.
 *
 * Byte Array (N+1) - Send data to a remote node
 *      0x01 | <data1> | <data1> | ... | <dataN>
 *
 * Vehicle Present (5) - Signal that a vehicle has been attached to the node
 *      0x02 | ID byte 1 (MSB) | ID byte 2 | ID byte 3 | ID byte 1 (LSB)
 *
 * Contained Vehciles Request (1) - Make request to remote node(s) to send all vehicles present
 *      0x03
 *
 * Remove Vehicle (5) - Signal that a vehicle attached to a node is no longer
 *      0x04 | ID byte 1 (MSB) | ID byte 2 | ID byte 3 | ID byte 1 (LSB)
 *
 */


template <typename ...T>
void Notify(std::vector<std::function<void(T...)>> lambdas, T... args)
{
    for(auto it = lambdas.cbegin() ; it != lambdas.cend() ; ++it) {
        (*it)(args...);
    }
}

template<const char*... A>
class _MACEDigiMeshWrapper;



class DigiMeshElement
{
private:

    std::unordered_map<int, uint64_t> m_VehicleIDToRadioAddr;
    std::mutex m_VehicleIDMutex;

    std::vector<int> m_ContainedVehicles;

public:
    std::vector<std::function<void(int, uint64_t)>> m_Handlers_NewRemoteVehicle;
    std::vector<std::function<void(int)>> m_Handlers_RemoteVehicleRemoved;
    std::vector<std::function<void(int, TransmitStatusTypes)>> m_Handlers_VehicleNotReached;

public:

    void AddInternalElement(int vehicleID) {

        if(m_VehicleIDToRadioAddr.find(vehicleID) != m_VehicleIDToRadioAddr.cend()){
            throw std::runtime_error("Vehicle of given ID already exists");
        }

        m_VehicleIDMutex.lock();
        m_VehicleIDToRadioAddr.insert({vehicleID, 0x00});
        m_VehicleIDMutex.unlock();

        m_ContainedVehicles.push_back(vehicleID);
    }

    uint64_t GetAddr(int ID)
    {
        return m_VehicleIDToRadioAddr.at(ID);
    }

    std::vector<int> ContainedIDs() const {
        return m_ContainedVehicles;
    }

    bool AddExternalElement(int vehicleID, uint64_t addr) {

        if(m_VehicleIDToRadioAddr.find(vehicleID) == m_VehicleIDToRadioAddr.end()) {
            m_VehicleIDMutex.lock();
            m_VehicleIDToRadioAddr.insert({vehicleID, addr});
            m_VehicleIDMutex.unlock();

            return true;
        }
        else {
            if(m_VehicleIDToRadioAddr[vehicleID] != addr) {
                throw std::runtime_error("Remote Vehicle ID passed to this node already exists");
            }

            return false;
        }
    }


    void AddHandler_NewRemoteVehicle(const std::function<void(int, uint64_t)> &lambda)
    {
        m_Handlers_NewRemoteVehicle.push_back(lambda);
    }


    void AddHandler_RemoteVehicleRemoved(const std::function<void(int)> &lambda)
    {

    }


    void AddHandler_Data(const std::function<void(const std::vector<uint8_t>& data)> &lambda)
    {

    }


    void AddHandler_VehicleTransmitError(const std::function<void(int vehicle, TransmitStatusTypes status)> &lambda)
    {

    }

};


template<const char* Type, const char*... Rest>
class _MACEDigiMeshWrapper<Type, Rest...> : public _MACEDigiMeshWrapper<Rest...>
{
private:

public:

    _MACEDigiMeshWrapper<Type, Rest...>()
    {
        _MACEDigiMeshWrapper<Rest...>::m_ElementMap.insert({Type, new DigiMeshElement()});
    }

    ~_MACEDigiMeshWrapper<Type, Rest...>()
    {

    }

};

template <>
class MACEWRAPPERSHARED_EXPORT _MACEDigiMeshWrapper<>
{
public:
    std::unordered_map<std::string, DigiMeshElement*> m_ElementMap;

public:

    _MACEDigiMeshWrapper<>() {

    }
};


template <const char*... A>
class MACEWRAPPERSHARED_EXPORT MACEDigiMeshWrapper : public _MACEDigiMeshWrapper<A...>
{
    using _MACEDigiMeshWrapper<A...>::m_ElementMap;
private:


    enum class PacketTypes
    {
        DATA = 0x01,
        VEHICLE_PRESENT = 0x02,
        CONTAINED_VECHILES_REQUEST = 0x03,
        REMOVE_VEHICLE = 0x04
    };

    static const char NI_NAME_VEHICLE_DELIMETER = '|';

    void* m_Radio;


    std::mutex m_NIMutex;

    std::thread *m_ScanThread;
    bool m_ShutdownScanThread;

    std::vector<std::function<void(const std::vector<uint8_t>&)>> m_Handlers_Data;

    std::string m_NodeName;

public:

    /**
     * @brief Constructor
     *
     * If no name is given, this node will set names according to the vehicles present
     * @param port Port to communicate with DigiMesh Radio
     * @param rate Baud Rate to communicate at
     * @param nameOfNode [""] Optional name of node
     * @param scanForVehicles [false] Indicate if this radio should scan for other MACE vehicles, or rely upon messages sent.
     */
    MACEDigiMeshWrapper(const std::string &port, DigiMeshBaudRates rate, const std::string &nameOfNode = "", bool scanForNodes = false);

    ~MACEDigiMeshWrapper();


    template <const char* T>
    void AddElement(const int ID) {
        this->m_ElementMap[T]->AddInternalElement(ID);
        send_vehicle_present_message(T, ID);
    }


    /**
     * @brief Add handler to be called when a new vehicle is added to the network
     * @param lambda Lambda function whoose parameters are the vehicle ID and node address of new vechile.
     */
    template <const char * T>
    void AddHandler_NewRemoteVehicle(const std::function<void(int, uint64_t)> &lambda)
    {
        m_ElementMap[T]->AddHandler_NewRemoteVehicle(lambda);
    }


    /**
     * @brief Add handler to be called when a new vehicle has been removed from the network
     * @param lambda Lambda function whoose parameters are the vehicle ID of removed vechile.
     */
    template <const char * T>
    void AddHandler_RemoteVehicleRemoved(const std::function<void(int)> &lambda)
    {
        m_ElementMap[T]->AddHandler_RemoteVehicleRemoved(lambda);
    }


    /**
     * @brief Add handler to be called when data has been received to this node
     * @param lambda Lambda function accepting an array of single byte values
     */
    void AddHandler_Data(const std::function<void(const std::vector<uint8_t>& data)> &lambda)
    {
        m_Handlers_Data.push_back(lambda);
    }


    /**
     * @brief Add handler to be called when tranmission to a vehicle failed for some reason.
     * @param lambda Lambda function to pass vehicle ID and status code
     */
    template <const char * T>
    void AddHandler_VehicleTransmitError(const std::function<void(int vehicle, TransmitStatusTypes status)> &lambda)
    {
        m_ElementMap[T]->AddHandler_VehicleTransmitError(lambda);
    }


    /**
     * @brief Send data to a vechile
     * @param destVechileID ID of vehicle
     * @param data Data to send
     * @throws std::runtime_error Thrown if no vehicle of given id is known.
     */
    template <const char* T>
    void SendData(const int &destVehicleID, const std::vector<uint8_t> &data)
    {
        //address to send to
        uint64_t addr = m_ElementMap[T]->GetAddr(destVehicleID);

        send_data_to_address(addr, data, [this, destVehicleID](const TransmitStatusTypes &status){

            if(status != TransmitStatusTypes::SUCCESS)
            {
                Notify<int, TransmitStatusTypes>(this->m_ElementMap[T]->m_Handlers_VehicleNotReached, destVehicleID, status);
            }
        });
    }


    /**
     * @brief Broadcast data to all nodes
     * @param data Data to broadcast out
     */
    void BroadcastData(const std::vector<uint8_t> &data);


private:

    void send_data_to_address(uint64_t addr, const std::vector<uint8_t> &data, const std::function<void(const TransmitStatusTypes &)> &cb);

    void run_scan();


    /**
     * @brief Logic to perform upon reception of a message
     * @param msg Message received
     */
    void on_message_received(const std::vector<uint8_t> &msg, uint64_t addr);


    /**
     * @brief Logic to perform when a vehicle is known about
     * @param vehicleID ID of vehicle
     * @param addr Digimesh node that vehicle is contained on. 0 if node is own address
     */
    void on_new_remote_vehicle(const char* elementType, const int vehicleID, const uint64_t &addr);


    /**
     * @brief Logic to perform when a vehicle is removed
     * @param vehicleID ID of vehicle
     */
    void on_remote_vehicle_removed(const int vehicleID);


    void send_vehicle_present_message(const char *elementName, const int vehicleID);

    void send_vehicle_removal_message(const int vehicleID);
};

#include "mace_digimesh_wrapper.hpp"

#endif // MACE_WRAPPER_H
