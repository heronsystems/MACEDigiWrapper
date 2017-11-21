#include "mace_digimesh_wrapper.h"

#include <sstream>
#include <iostream>
#include <vector>

template <typename ...T>
void Notify(std::vector<std::function<void(T...)>> lambdas, T... args)
{
    for(auto it = lambdas.cbegin() ; it != lambdas.cend() ; ++it) {
        (*it)(args...);
    }
}


MACEDigiMeshWrapper::MACEDigiMeshWrapper(const std::string &port, DigiMeshBaudRates rate) :
    m_Radio(port, rate),
    //m_ScanThread([this](){run_scan();}),
    m_ShutdownScanThread(false)
{
    m_NIMutex.lock();
    m_Radio.SetATParameterAsync<ATData::Integer<uint8_t>>("AP", ATData::Integer<uint8_t>(1), [this](){
        m_Radio.SetATParameterAsync<ATData::String>("NI", "-", [this](){
            m_NIMutex.unlock();
        });
    });

    m_Radio.AddMessageHandler([this](const ATData::Message &a){this->on_message_received(a);});


    //broadcast a request for everyone to send their vehicles
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::CONTAINED_VECHILES_REQUEST);
    m_Radio.SendMessage(packet);
}


MACEDigiMeshWrapper::~MACEDigiMeshWrapper() {
    m_Radio.SetATParameterAsync<ATData::String>("AP", "-");
    m_ShutdownScanThread = true;
}


/**
 * @brief Adds a vechile to the network.
 * @param ID ID of vehicle to add
 */
void MACEDigiMeshWrapper::AddVehicle(const int &ID)
{
    m_VehicleIDMutex.lock();
    m_VehicleIDToRadioAddr.insert({ID, 0x00});
    m_VehicleIDMutex.unlock();

    m_ContainedVehicles.push_back(ID);

    m_NIMutex.lock();
    m_Radio.GetATParameterAsync<ATData::String, ShutdownFirstResponse>("NI", [this, ID](const std::vector<ATData::String> &currNIarr) {
       ATData::String currNI = currNIarr.at(0) ;
       bool changeMade = false;
       if(currNI == "-") {
           currNI.assign(std::to_string(ID));
           changeMade = true;
       }
       else {
           if(currNI.find(ID) == std::string::npos) {
               currNI += NI_NAME_VEHICLE_DELIMETER + std::to_string(ID);
               changeMade = true;
           }
       }

       if(changeMade == true) {
           m_Radio.SetATParameterAsync("NI", currNI, [this](){m_NIMutex.unlock();});
       }
    });

    send_vehicle_message(ID);
}


/**
 * @brief Add handler to be called when a new vehicle is added to the network
 * @param lambda Lambda function whoose parameters are the vehicle ID and node address of new vechile.
 */
void MACEDigiMeshWrapper::AddHandler_NewRemoteVehicle(const std::function<void(int, uint64_t)> &lambda)
{
    m_Handlers_NewRemoteVehicle.push_back(lambda);
}


/**
 * @brief Add handler to be called when data has been received to this node
 * @param lambda Lambda function accepting an array of single byte values
 */
void MACEDigiMeshWrapper::AddHandler_Data(const std::function<void(const std::vector<uint8_t>& data)> &lambda)
{
    m_Handlers_Data.push_back(lambda);
}


/**
 * @brief Send data to a vechile
 * @param destVechileID ID of vehicle
 * @param data Data to send
 * @throws std::runtime_error Thrown if no vechile of given id is known.
 */
void MACEDigiMeshWrapper::SendData(const int &destVechileID, const std::vector<uint8_t> &data)
{
    if(m_VehicleIDToRadioAddr.find(destVechileID) == m_VehicleIDToRadioAddr.end()) {
        throw std::runtime_error("No known vehicle");
    }

    //address to send to
    uint64_t addr = m_VehicleIDToRadioAddr.at(destVechileID);
    if(addr == 0) {
        Notify<const std::vector<uint8_t>&>(m_Handlers_Data, data);
    }

    //construct packet, putting the packet type at head
    std::vector<uint8_t> packet = {(uint8_t)PacketTypes::DATA};
    for(int i = 0 ; i < data.size() ; i++) {
        packet.push_back(data.at(i));
    }

    m_Radio.SendMessage(packet, this->m_VehicleIDToRadioAddr[destVechileID]);
}


/**
 * @brief Broadcast data to all nodes
 * @param data Data to broadcast out
 */
void MACEDigiMeshWrapper::BroadcastData(const std::vector<uint8_t> &data)
{
    //construct packet, putting the packet type at head
    std::vector<uint8_t> packet = {(uint8_t)PacketTypes::DATA};
    for(int i = 0 ; i < data.size() ; i++) {
        packet.push_back(data.at(i));
    }

    m_Radio.SendMessage(packet);
}

void MACEDigiMeshWrapper::run_scan() {
    while(true){
        if(m_ShutdownScanThread == true) {
            return;
        }

        std::vector<ATData::NodeDiscovery> nodes = m_Radio.GetATParameterSync<ATData::NodeDiscovery, CollectAfterTimeout>("ND", CollectAfterTimeout(15000));

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
}


/**
 * @brief Logic to perform upon reception of a message
 * @param msg Message received
 */
void MACEDigiMeshWrapper::on_message_received(const ATData::Message &msg)
{
    PacketTypes packetType = (PacketTypes)msg.data.at(0);
    switch(packetType) {
        case PacketTypes::DATA:
            {
            std::vector<uint8_t> data;
            for(int i = 1 ; i < msg.data.size() ; i++) {
                data.push_back(msg.data.at(i));
            }
            Notify<const std::vector<uint8_t>&>(m_Handlers_Data, data);
            break;
        }
        case PacketTypes::NEW_VECHILE:
        {
            int vehicleID = 0;
            for(int i = 0 ; i < 4 ; i++) {
                vehicleID |= (((uint64_t)msg.data[1+i]) << (8*(3-i)));
            }
            on_new_remote_vehicle(vehicleID, msg.addr);
            break;
        }
        case PacketTypes::CONTAINED_VECHILES_REQUEST:
        {
            for(auto it = m_ContainedVehicles.cbegin() ; it != m_ContainedVehicles.cend() ; ++it) {
                send_vehicle_message(*it);
            }
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
void MACEDigiMeshWrapper::on_new_remote_vehicle(const int vehicleID, const uint64_t &addr)
{
    //if the vehicle of given id doesn't exists, add it
    if(m_VehicleIDToRadioAddr.find(vehicleID) == m_VehicleIDToRadioAddr.end()) {
        m_VehicleIDMutex.lock();
        m_VehicleIDToRadioAddr.insert({vehicleID, addr});
        m_VehicleIDMutex.unlock();

        Notify<int, uint64_t>(m_Handlers_NewRemoteVehicle, vehicleID, addr);
    }
}

void MACEDigiMeshWrapper::send_vehicle_message(const int vehicleID)
{
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::NEW_VECHILE);
    for(size_t i = 0 ; i < 4 ; i++) {
        uint64_t a = (vehicleID & (0xFFll << (8*(3-i)))) >> (8*(3-i));
        packet.push_back((char)a);
    }
    m_Radio.SendMessage(packet);
}


