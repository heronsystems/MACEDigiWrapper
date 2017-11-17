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

class MACEWRAPPERSHARED_EXPORT MACEDigiMeshWrapper
{
private:

    static const char NI_NAME_VEHICLE_DELIMETER = '|';

    DigiMeshRadio m_Radio;

    std::unordered_map<int, uint64_t> m_VehicleIDToRadioAddr;
    std::mutex m_VehicleIDMutex;

    std::mutex m_NIMutex;

    std::thread m_ScanThread;
    bool m_ShutdownScanThread;

    std::vector<std::function<void(int, uint64_t)>> m_Handlers_NewRemoteVehicle;

public:
    MACEDigiMeshWrapper(const std::string &port, DigiMeshBaudRates rate);

    ~MACEDigiMeshWrapper();

    /**
     * @brief Adds a vechile to the network.
     * @param ID ID of vehicle to add
     */
    void AddVehicle(const int &ID);


    void AddHandler_NewRemoteVehicle(const std::function<void(int, uint64_t)> &);

private:

    void run_scan();
};

#endif // MACE_WRAPPER_H
