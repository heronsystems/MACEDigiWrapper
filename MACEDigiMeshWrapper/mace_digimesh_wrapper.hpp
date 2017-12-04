#include "mace_digimesh_wrapper.h"

#include <sstream>
#include <iostream>
#include <vector>
#include <mutex>

#include "digimesh_radio.h"

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}










template<const char* ... T>
MACEDigiMeshWrapper<T...>::~MACEDigiMeshWrapper() {
    if(m_NodeName == ""){
        ((DigiMeshRadio*)m_Radio)->SetATParameterAsync<ATData::String>("AP", "-");
    }
    m_ShutdownScanThread = true;
    m_ScanThread->join();
    delete m_ScanThread;
}




/**
 * @brief Logic to perform when a vehicle is known about
 * @param vehicleID ID of vehicle
 * @param addr Digimesh node that vehicle is contained on. 0 if node is own address
 */
template<const char* ... T>
void MACEDigiMeshWrapper<T...>::on_new_remote_vehicle(const char *elementType, const int vehicleID, const uint64_t &addr)
{
    if(m_ElementMap.find(elementType) == this->m_ElementMap.cend()) {
        throw std::runtime_error("Unexpected element type, are parameters the same accross instances?");
    }
    if(m_ElementMap[elementType]->AddExternalElement(vehicleID, addr))
    {
        Notify<int, uint64_t>(m_ElementMap[elementType]->m_Handlers_NewRemoteVehicle, vehicleID, addr);
    }
}


/**
 * @brief Logic to perform when a vehicle is removed
 * @param vehicleID ID of vehicle
 */
template<const char* ... T>
void MACEDigiMeshWrapper<T...>::on_remote_vehicle_removed(const int vehicleID)
{
    /*
    //if the vehicle of given id doesn't exists, add it
    if(m_VehicleIDToRadioAddr.find(vehicleID) != m_VehicleIDToRadioAddr.end()) {
        m_VehicleIDMutex.lock();
        m_VehicleIDToRadioAddr.erase(vehicleID);
        m_VehicleIDMutex.unlock();

        Notify<int>(m_Handlers_RemoteVehicleRemoved, vehicleID);
    }
    */
}

template<const char* ... T>
void MACEDigiMeshWrapper<T...>::send_vehicle_present_message(const char *elementName, const int vehicleID)
{
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::VEHICLE_PRESENT);

    size_t pos = 0;
    do
    {
        packet.push_back(elementName[pos]);
        pos++;
    }
    while(elementName[pos] != '\0');
    packet.push_back('\0');

    for(size_t i = 0 ; i < 4 ; i++) {
        uint64_t a = (vehicleID & (0xFFll << (8*(3-i)))) >> (8*(3-i));
        packet.push_back((char)a);
    }
    ((DigiMeshRadio*)m_Radio)->SendMessage(packet);
}

template<const char* ... T>
void MACEDigiMeshWrapper<T...>::send_vehicle_removal_message(const int vehicleID)
{
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::REMOVE_VEHICLE);
    for(size_t i = 0 ; i < 4 ; i++) {
        uint64_t a = (vehicleID & (0xFFll << (8*(3-i)))) >> (8*(3-i));
        packet.push_back((char)a);
    }
    ((DigiMeshRadio*)m_Radio)->SendMessage(packet);
}


