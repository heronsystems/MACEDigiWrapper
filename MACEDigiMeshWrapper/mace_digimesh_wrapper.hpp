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


/*
template <const char* ...T>
void variadicExpand(const std::function<void(const char* element)> &lambda)
{

}
*/

template<const char* T>
void variadicExpand(std::function<void(const char* element)> lambda)
{
    lambda(T);
}

template<const char* Head, const char* Next, const char* ...Tail>
void variadicExpand(std::function<void(const char* element)> lambda)
{
    lambda(Head);
    variadicExpand<Next, Tail...>(lambda);
}






template<const char* ... T>
MACEDigiMeshWrapper<T...>::MACEDigiMeshWrapper(const std::string &port, DigiMeshBaudRates rate, const std::string &nameOfNode, bool scanForNodes) :
    m_ShutdownScanThread(false),
    m_NodeName(nameOfNode)
{
    m_Radio = new DigiMeshRadio(port, rate);

    m_NIMutex.lock();
    ((DigiMeshRadio*)m_Radio)->SetATParameterAsync<ATData::Integer<uint8_t>>("AP", ATData::Integer<uint8_t>(1), [this](){
        ((DigiMeshRadio*)m_Radio)->SetATParameterAsync<ATData::String>("NI", m_NodeName.c_str(), [this](){
            m_NIMutex.unlock();
        });


    });

    if(scanForNodes == true) {
        m_ScanThread = new std::thread([this](){run_scan();});
    }

    ((DigiMeshRadio*)m_Radio)->AddMessageHandler([this](const ATData::Message &a){this->on_message_received(a.data, a.addr);});


    //broadcast a request for everyone to send their Elements
    variadicExpand<T...>([this](const char* element) {

        std::vector<uint8_t> packet;
        packet.push_back((uint8_t)PacketTypes::CONTAINED_VECHILES_REQUEST);

        size_t pos = 0;
        do
        {
            packet.push_back(element[pos]);
            pos++;
        }
        while(element[pos] != '\0');
        packet.push_back('\0');

        ((DigiMeshRadio*)m_Radio)->SendMessage(packet);

    });

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
 * @brief Broadcast data to all nodes
 * @param data Data to broadcast out
 */
template<const char* ... T>
void MACEDigiMeshWrapper<T...>::BroadcastData(const std::vector<uint8_t> &data)
{
    //construct packet, putting the packet type at head
    std::vector<uint8_t> packet = {(uint8_t)PacketTypes::DATA};
    for(int i = 0 ; i < data.size() ; i++) {
        packet.push_back(data.at(i));
    }

    ((DigiMeshRadio*)m_Radio)->SendMessage(packet);
}


template<const char* ... T>
void MACEDigiMeshWrapper<T...>::send_data_to_address(uint64_t addr, const std::vector<uint8_t> &data, const std::function<void(const TransmitStatusTypes &)> &cb)
{

    //if sending to self, notify self
    if(addr == 0) {
        Notify<const std::vector<uint8_t>&>(m_Handlers_Data, data);
    }

    //construct packet, putting the packet type at head
    std::vector<uint8_t> packet = {(uint8_t)PacketTypes::DATA};
    for(int i = 0 ; i < data.size() ; i++) {
        packet.push_back(data.at(i));
    }

    ((DigiMeshRadio*)m_Radio)->SendMessage(packet, addr, [cb](const ATData::TransmitStatus status){
        cb(status.status);
    });
}

template<const char* ... T>
void MACEDigiMeshWrapper<T...>::run_scan() {
    /*
    while(true){
        if(m_ShutdownScanThread == true) {
            return;
        }

        std::vector<ATData::NodeDiscovery> nodes = ((DigiMeshRadio*)m_Radio)->GetATParameterSync<ATData::NodeDiscovery, CollectAfterTimeout>("ND", CollectAfterTimeout(15000));

        for(std::vector<ATData::NodeDiscovery>::const_iterator it = nodes.cbegin() ; it != nodes.cend() ; ++it) {
            ATData::NodeDiscovery node = *it;

            std::string NI = node.NI;

            //delemet out the NI string to get the vehicles that it has
            std::vector<std::string> vehicles;
            std::istringstream f(NI);
            std::string s;
            while (getline(f, s, NI_NAME_VEHICLE_DELIMETER)) {
                vehicles.push_back(s);
            }

            //iterate through each vehicle contained on the node
            for(auto vehicleIT = vehicles.cbegin() ; vehicleIT != vehicles.cend() ; ++vehicleIT) {

                int num;
                try {
                    num = std::stoi(*vehicleIT);
                }
                catch(const std::invalid_argument& oor) {
                    continue;
                }
                catch(const std::out_of_range& oor) {
                    continue;
                }


                on_new_remote_vehicle(num, node.addr);
            }

        }
    }
    */
}


/**
 * @brief Logic to perform upon reception of a message
 * @param msg Message received
 */
template<const char* ... T>
void MACEDigiMeshWrapper<T...>::on_message_received(const std::vector<uint8_t> &msg, uint64_t addr)
{
    PacketTypes packetType = (PacketTypes)msg.at(0);
    switch(packetType) {
        case PacketTypes::DATA:
            {
            std::vector<uint8_t> data;
            for(int i = 1 ; i < msg.size() ; i++) {
                data.push_back(msg.at(i));
            }
            Notify<const std::vector<uint8_t>&>(m_Handlers_Data, data);
            break;
        }
        case PacketTypes::VEHICLE_PRESENT:
        {
            std::string element;
            int pos = 1;
            while(msg[pos] != '\0') {
                element += msg[pos];
                pos++;
            }
            int vehicleID = 0;
            for(int i = 0 ; i < 4 ; i++) {
                vehicleID |= (((uint64_t)msg[pos+1+i]) << (8*(3-i)));
            }
            on_new_remote_vehicle(element.c_str(), vehicleID, addr);
            break;
        }
        case PacketTypes::CONTAINED_VECHILES_REQUEST:
        {
            std::string element;
            int pos = 1;
            while(msg[pos] != '\0') {
                element += msg[pos];
                pos++;
            }

            std::vector<int> contained = this->m_ElementMap[element]->ContainedIDs();

            for(auto it = contained.cbegin() ; it != contained.cend() ; ++it) {
                send_vehicle_present_message(element.c_str(), *it);
            }
            break;
        }
        case PacketTypes::REMOVE_VEHICLE:
        {
            int vehicleID = 0;
            for(int i = 0 ; i < 4 ; i++) {
                vehicleID |= (((uint64_t)msg[1+i]) << (8*(3-i)));
            }
            on_remote_vehicle_removed(vehicleID);
            break;
        }
        default:
            throw std::runtime_error("Unknown packet type received over digimesh network");
    }
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


