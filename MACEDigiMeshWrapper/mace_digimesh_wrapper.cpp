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


void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}


MACEDigiMeshWrapper::MACEDigiMeshWrapper(const std::string &port, DigiMeshBaudRates rate, const std::string &nameOfNode, bool scanForNodes) :
    m_Radio(port, rate),
    m_ShutdownScanThread(false),
    m_NodeName(nameOfNode)
{
    m_NIMutex.lock();
    m_Radio.SetATParameterAsync<ATData::Integer<uint8_t>>("AP", ATData::Integer<uint8_t>(1), [this](){
        m_Radio.SetATParameterAsync<ATData::String>("NI", m_NodeName.c_str(), [this](){
            m_NIMutex.unlock();
        });


    });

    if(scanForNodes == true) {
        m_ScanThread = new std::thread([this](){run_scan();});
    }

    m_Radio.AddMessageHandler([this](const ATData::Message &a){this->on_message_received(a);});


    //broadcast a request for everyone to send their vehicles
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::CONTAINED_VECHILES_REQUEST);
    m_Radio.SendMessage(packet);
}


MACEDigiMeshWrapper::~MACEDigiMeshWrapper() {
    if(m_NodeName == ""){
        m_Radio.SetATParameterAsync<ATData::String>("AP", "-");
    }
    m_ShutdownScanThread = true;
    m_ScanThread->join();
    delete m_ScanThread;
}


/**
 * @brief Adds a vechile to the network.
 * @param ID ID of vehicle to add
 * @throws std::runtime_error Thrown if given vehicleID already exists
 */
void MACEDigiMeshWrapper::AddVehicle(const int &ID)
{
    if(m_VehicleIDToRadioAddr.find(ID) != m_VehicleIDToRadioAddr.cend()){
        throw std::runtime_error("Vehicle of given ID already exists");
    }

    m_VehicleIDMutex.lock();
    m_VehicleIDToRadioAddr.insert({ID, 0x00});
    m_VehicleIDMutex.unlock();

    m_ContainedVehicles.push_back(ID);

    //If our node doesn't have a name, set the name according to the vehicles present
    if(m_NodeName == "")
    {
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
    }

    send_vehicle_present_message(ID);
}


/**
 * @brief Removes a vehicle from the network.
 *
 * If given ID doesn't coorilate to any vehicle, no action is done
 * @param ID ID of vehicle
 */
void MACEDigiMeshWrapper::RemoveVehicle(const int &ID)
{
    if(m_VehicleIDToRadioAddr.find(ID) == m_VehicleIDToRadioAddr.cend()){
        return;
    }

    m_VehicleIDMutex.lock();
    m_VehicleIDToRadioAddr.erase(ID);
    m_VehicleIDMutex.unlock();

    //If our node doesn't have a name, remove the vehicle ID from NI string
    if(m_NodeName == "")
    {
        m_NIMutex.lock();
        m_Radio.GetATParameterAsync<ATData::String, ShutdownFirstResponse>("NI", [this, ID](const std::vector<ATData::String> &currNIarr) {
           ATData::String currNI = currNIarr.at(0) ;

           std::string str_ID = std::to_string(ID);
           replaceAll(currNI, str_ID + NI_NAME_VEHICLE_DELIMETER, "");
           replaceAll(currNI, NI_NAME_VEHICLE_DELIMETER + str_ID , "");
           replaceAll(currNI, str_ID, "");

           if(currNI == "") {
               currNI = "-";
           }

           if(currNIarr.at(0) != currNI) {
               m_Radio.SetATParameterAsync("NI", currNI, [this](){m_NIMutex.unlock();});
           }
        });
    }

    send_vehicle_removal_message(ID);
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
 * @brief Add handler to be called when a new vehicle has been removed from the network
 * @param lambda Lambda function whoose parameters are the vehicle ID of removed vechile.
 */
void MACEDigiMeshWrapper::AddHandler_RemoteVehicleRemoved(const std::function<void(int)> &lambda)
{
    m_Handlers_RemoteVehicleRemoved.push_back(lambda);
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
 * @throws std::runtime_error Thrown if no vehicle of given id is known.
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
        case PacketTypes::VEHICLE_PRESENT:
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
                send_vehicle_present_message(*it);
            }
            break;
        }
        case PacketTypes::REMOVE_VEHICLE:
        {
            int vehicleID = 0;
            for(int i = 0 ; i < 4 ; i++) {
                vehicleID |= (((uint64_t)msg.data[1+i]) << (8*(3-i)));
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
void MACEDigiMeshWrapper::on_new_remote_vehicle(const int vehicleID, const uint64_t &addr)
{
    //if the vehicle of given id doesn't exists, add it
    if(m_VehicleIDToRadioAddr.find(vehicleID) == m_VehicleIDToRadioAddr.end()) {
        m_VehicleIDMutex.lock();
        m_VehicleIDToRadioAddr.insert({vehicleID, addr});
        m_VehicleIDMutex.unlock();

        Notify<int, uint64_t>(m_Handlers_NewRemoteVehicle, vehicleID, addr);
    }
    else {
        if(m_VehicleIDToRadioAddr[vehicleID] != addr) {
            throw std::runtime_error("Remote Vehicle ID passed to this node already exists");
        }
    }
}


/**
 * @brief Logic to perform when a vehicle is removed
 * @param vehicleID ID of vehicle
 */
void MACEDigiMeshWrapper::on_remote_vehicle_removed(const int vehicleID)
{
    //if the vehicle of given id doesn't exists, add it
    if(m_VehicleIDToRadioAddr.find(vehicleID) != m_VehicleIDToRadioAddr.end()) {
        m_VehicleIDMutex.lock();
        m_VehicleIDToRadioAddr.erase(vehicleID);
        m_VehicleIDMutex.unlock();

        Notify<int>(m_Handlers_RemoteVehicleRemoved, vehicleID);
    }
}

void MACEDigiMeshWrapper::send_vehicle_present_message(const int vehicleID)
{
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::VEHICLE_PRESENT);
    for(size_t i = 0 ; i < 4 ; i++) {
        uint64_t a = (vehicleID & (0xFFll << (8*(3-i)))) >> (8*(3-i));
        packet.push_back((char)a);
    }
    m_Radio.SendMessage(packet);
}

void MACEDigiMeshWrapper::send_vehicle_removal_message(const int vehicleID)
{
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::REMOVE_VEHICLE);
    for(size_t i = 0 ; i < 4 ; i++) {
        uint64_t a = (vehicleID & (0xFFll << (8*(3-i)))) >> (8*(3-i));
        packet.push_back((char)a);
    }
    m_Radio.SendMessage(packet);
}


