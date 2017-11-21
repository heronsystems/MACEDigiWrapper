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
 * Byte Array                  :  0x01 | <data1> | <data1> | ... | <dataN>
 * New Vehicle                 :  0x02 | ID byte 1 (MSB) | ID byte 2 | ID byte 3 | ID byte 1 (LSB)
 * Contained Vehciles Request  :  0x03 |
 */
class MACEWRAPPERSHARED_EXPORT MACEDigiMeshWrapper
{
private:


    enum class PacketTypes
    {
        DATA = 0x01,
        NEW_VECHILE = 0x02,
        CONTAINED_VECHILES_REQUEST = 0x03
    };

    static const char NI_NAME_VEHICLE_DELIMETER = '|';

    DigiMeshRadio m_Radio;

    std::vector<int> m_ContainedVehicles;
    std::unordered_map<int, uint64_t> m_VehicleIDToRadioAddr;
    std::mutex m_VehicleIDMutex;

    std::mutex m_NIMutex;

    std::thread m_ScanThread;
    bool m_ShutdownScanThread;

    std::vector<std::function<void(int, uint64_t)>> m_Handlers_NewRemoteVehicle;
    std::vector<std::function<void(const std::vector<uint8_t>&)>> m_Handlers_Data;

public:
    MACEDigiMeshWrapper(const std::string &port, DigiMeshBaudRates rate);

    ~MACEDigiMeshWrapper();

    /**
     * @brief Adds a vechile to the network.
     * @param ID ID of vehicle to add
     */
    void AddVehicle(const int &ID);


    /**
     * @brief Add handler to be called when a new vehicle is added to the network
     * @param lambda Lambda function whoose parameters are the vehicle ID and node address of new vechile.
     */
    void AddHandler_NewRemoteVehicle(const std::function<void(int, uint64_t)> &lambda);


    /**
     * @brief Add handler to be called when data has been received to this node
     * @param lambda Lambda function accepting an array of single byte values
     */
    void AddHandler_Data(const std::function<void(const std::vector<uint8_t>& data)> &lambda);


    /**
     * @brief Send data to a vechile
     * @param destVechileID ID of vehicle
     * @param data Data to send
     * @throws std::runtime_error Thrown if no vechile of given id is known.
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


    void send_vehicle_message(const int vehicleID);
};

#endif // MACE_WRAPPER_H
