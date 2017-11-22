#ifndef MACE_WRAPPER_H
#define MACE_WRAPPER_H

#include "macewrapper_global.h"

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <stdint.h>

#include "digi_mesh_baud_rates.h"
#include "digimesh_radio.h"

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
class MACEWRAPPERSHARED_EXPORT MACEDigiMeshWrapper
{
private:


    enum class PacketTypes
    {
        DATA = 0x01,
        VEHICLE_PRESENT = 0x02,
        CONTAINED_VECHILES_REQUEST = 0x03,
        REMOVE_VEHICLE = 0x04
    };

    static const char NI_NAME_VEHICLE_DELIMETER = '|';

    DigiMeshRadio m_Radio;

    std::vector<int> m_ContainedVehicles;
    std::unordered_map<int, uint64_t> m_VehicleIDToRadioAddr;
    std::mutex m_VehicleIDMutex;

    std::mutex m_NIMutex;

    std::thread *m_ScanThread;
    bool m_ShutdownScanThread;

    std::vector<std::function<void(int, uint64_t)>> m_Handlers_NewRemoteVehicle;
    std::vector<std::function<void(const std::vector<uint8_t>&)>> m_Handlers_Data;
    std::vector<std::function<void(int)>> m_Handlers_RemoteVehicleRemoved;

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

    /**
     * @brief Adds a vechile to the network.
     * @param ID ID of vehicle to add
     * @throws std::runtime_error Thrown if given vehicleID already exists
     */
    void AddVehicle(const int &ID);


    /**
     * @brief Removes a vehicle from the network.
     *
     * If given ID doesn't coorilate to any vehicle, no action is done
     * @param ID ID of vehicle
     */
    void RemoveVehicle(const int &ID);


    /**
     * @brief Add handler to be called when a new vehicle is added to the network
     * @param lambda Lambda function whoose parameters are the vehicle ID and node address of new vechile.
     */
    void AddHandler_NewRemoteVehicle(const std::function<void(int, uint64_t)> &lambda);


    /**
     * @brief Add handler to be called when a new vehicle has been removed from the network
     * @param lambda Lambda function whoose parameters are the vehicle ID of removed vechile.
     */
    void AddHandler_RemoteVehicleRemoved(const std::function<void(int)> &lambda);


    /**
     * @brief Add handler to be called when data has been received to this node
     * @param lambda Lambda function accepting an array of single byte values
     */
    void AddHandler_Data(const std::function<void(const std::vector<uint8_t>& data)> &lambda);


    /**
     * @brief Send data to a vechile
     * @param destVechileID ID of vehicle
     * @param data Data to send
     * @throws std::runtime_error Thrown if no vehicle of given id is known.
     */
    void SendData(const int &destVechileID, const std::vector<uint8_t> &data);


    /**
     * @brief Broadcast data to all nodes
     * @param data Data to broadcast out
     */
    void BroadcastData(const std::vector<uint8_t> &data);


private:

    void run_scan();


    /**
     * @brief Logic to perform upon reception of a message
     * @param msg Message received
     */
    void on_message_received(const ATData::Message &msg);


    /**
     * @brief Logic to perform when a vehicle is known about
     * @param vehicleID ID of vehicle
     * @param addr Digimesh node that vehicle is contained on. 0 if node is own address
     */
    void on_new_remote_vehicle(const int vehicleID, const uint64_t &addr);


    /**
     * @brief Logic to perform when a vehicle is removed
     * @param vehicleID ID of vehicle
     */
    void on_remote_vehicle_removed(const int vehicleID);


    void send_vehicle_present_message(const int vehicleID);

    void send_vehicle_removal_message(const int vehicleID);
};

#endif // MACE_WRAPPER_H
