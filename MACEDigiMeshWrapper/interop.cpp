#include "interop.h"

#include "digimesh_radio.h"

/**
 * @brief Constructor
 *
 * If no name is given, this node will set names according to the vehicles present
 * @param port Port to communicate with DigiMesh Radio
 * @param rate Baud Rate to communicate at
 * @param nameOfNode [""] Optional name of node
 * @param scanForVehicles [false] Indicate if this radio should scan for other MACE vehicles, or rely upon messages sent.
 */
Interop::Interop(const std::string &port, DigiMeshBaudRates rate, const std::string &nameOfNode, bool scanForNodes) :
    m_NodeName(nameOfNode)
{
    m_Radio = new DigiMeshRadio(port, rate);

    m_NIMutex.lock();
    ((DigiMeshRadio*)m_Radio)->SetATParameterAsync<ATData::Integer<uint8_t>>("AP", ATData::Integer<uint8_t>(1), [this](){
        ((DigiMeshRadio*)m_Radio)->SetATParameterAsync<ATData::String>("NI", m_NodeName.c_str(), [this](){
            m_NIMutex.unlock();
        });


    });

    ((DigiMeshRadio*)m_Radio)->AddMessageHandler([this](const ATData::Message &a){this->on_message_received(a.data, a.addr);});
}


Interop::~Interop()
{
    if(m_NodeName == ""){
        ((DigiMeshRadio*)m_Radio)->SetATParameterAsync<ATData::String>("AP", "-");
    }
}



/**
 * @brief Add handler to be called when data has been received to this node
 * @param lambda Lambda function accepting an array of single byte values
 */
void Interop::AddHandler_Data(const std::function<void (const std::vector<uint8_t> &)> &lambda)
{
    m_Handlers_Data.push_back(lambda);
}


/**
 * @brief Broadcast data to all nodes
 * @param data Data to broadcast out
 */
void Interop::BroadcastData(const std::vector<uint8_t> &data)
{
    //construct packet, putting the packet type at head
    std::vector<uint8_t> packet = {(uint8_t)PacketTypes::DATA};
    for(int i = 0 ; i < data.size() ; i++) {
        packet.push_back(data.at(i));
    }

    ((DigiMeshRadio*)m_Radio)->SendMessage(packet);
}


void Interop::RequestContainedVehicles(const char *component)
{
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::CONTAINED_VECHILES_REQUEST);

    size_t pos = 0;
    do
    {
        packet.push_back(component[pos]);
        pos++;
    }
    while(component[pos] != '\0');
    packet.push_back('\0');

    ((DigiMeshRadio*)m_Radio)->SendMessage(packet);
}


void Interop::SendDataToAddress(uint64_t addr, const std::vector<uint8_t> &data, const std::function<void(const TransmitStatusTypes &)> &cb)
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


/**
 * @brief Logic to perform upon reception of a message
 * @param msg Message received
 */
void Interop::on_message_received(const std::vector<uint8_t> &msg, uint64_t addr)
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
        case PacketTypes::COMPONENT_ITEM_PRESENT:
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
            onNewRemoteComponentItem(element.c_str(), vehicleID, addr);
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

            std::vector<int> contained = RetrieveComponentItems(element.c_str());

            for(auto it = contained.cbegin() ; it != contained.cend() ; ++it) {
                send_item_present_message(element.c_str(), *it);
            }
            break;
        }
        case PacketTypes::REMOVE_COMPONENT_ITEM:
        {
            std::string componentName;
            int pos = 1;
            while(msg[pos] != '\0') {
                componentName += msg[pos];
                pos++;
            }

            int vehicleID = 0;
            for(int i = 0 ; i < 4 ; i++) {
                vehicleID |= (((uint64_t)msg[pos+1+i]) << (8*(3-i)));
            }
            onRemovedRemoteComponentItem(componentName.c_str(), vehicleID);
            break;\
        }
        default:
            throw std::runtime_error("Unknown packet type received over digimesh network");
    }
}


void Interop::send_item_present_message(const char *componentName, const int vehicleID)
{
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::COMPONENT_ITEM_PRESENT);

    size_t pos = 0;
    do
    {
        packet.push_back(componentName[pos]);
        pos++;
    }
    while(componentName[pos] != '\0');
    packet.push_back('\0');

    for(size_t i = 0 ; i < 4 ; i++) {
        uint64_t a = (vehicleID & (0xFFll << (8*(3-i)))) >> (8*(3-i));
        packet.push_back((char)a);
    }
    ((DigiMeshRadio*)m_Radio)->SendMessage(packet);
}


void Interop::send_item_remove_message(const char *componentName, const int vehicleID)
{
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)PacketTypes::REMOVE_COMPONENT_ITEM);

    size_t pos = 0;
    do
    {
        packet.push_back(componentName[pos]);
        pos++;
    }
    while(componentName[pos] != '\0');
    packet.push_back('\0');

    for(size_t i = 0 ; i < 4 ; i++) {
        uint64_t a = (vehicleID & (0xFFll << (8*(3-i)))) >> (8*(3-i));
        packet.push_back((char)a);
    }
    ((DigiMeshRadio*)m_Radio)->SendMessage(packet);
}
