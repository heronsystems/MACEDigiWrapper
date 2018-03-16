#include "interop_component.h"

/**
 * @brief Constructor
 *
 * If no name is given, this node will set names according to the vehicles present
 * @param port Port to communicate with DigiMesh Radio
 * @param rate Baud Rate to communicate at
 * @param nameOfNode [""] Optional name of node
 * @param scanForVehicles [false] Indicate if this radio should scan for other MACE vehicles, or rely upon messages sent.
 */
InteropComponent::InteropComponent(const std::string &port, DigiMeshBaudRates rate, const std::string &nameOfNode, bool scanForNodes)
    : Interop(port, rate, nameOfNode, scanForNodes)
{

}

void InteropComponent::AddComponentItem(const char* component, const int ID)
{
    GetComponent(component)->AddInternalItem(ID);
    send_item_present_message(component, ID);
}


void InteropComponent::RemoveComponentItem(const char* component, const int ID)
{
    GetComponent(component)->RemoveInternalItem(ID);
    send_item_remove_message(component, ID);
}


/**
 * @brief Add handler to be called when a new vehicle is added to the network
 * @param lambda Lambda function whoose parameters are the vehicle ID and node address of new vechile.
 */
void InteropComponent::AddHandler_NewRemoteComponentItem(const char* component, const std::function<void(int, uint64_t)> &lambda)
{
    if(m_Handlers_NewRemoteVehicle.find(component) == m_Handlers_NewRemoteVehicle.cend()) {
        m_Handlers_NewRemoteVehicle.insert({component, {}});
    }
    m_Handlers_NewRemoteVehicle.at(component).push_back(lambda);
}


/**
 * @brief Add handler to be called when a new vehicle has been removed from the network
 * @param lambda Lambda function whoose parameters are the vehicle ID of removed vechile.
 */
void InteropComponent::AddHandler_RemoteComponentItemRemoved(const char* component, const std::function<void(int)> &lambda)
{
    if(m_Handlers_RemoteVehicleRemoved.find(component) == m_Handlers_RemoteVehicleRemoved.cend()) {
        m_Handlers_RemoteVehicleRemoved.insert({component, {}});
    }
    m_Handlers_RemoteVehicleRemoved.at(component).push_back(lambda);
}


/**
 * @brief Add handler to be called when tranmission to a vehicle failed for some reason.
 * @param lambda Lambda function to pass vehicle ID and status code
 */
void InteropComponent::AddHandler_ComponentItemTransmitError(const char* component, const std::function<void(int vehicle, TransmitStatusTypes status)> &lambda)
{
    if(m_Handlers_VehicleNotReached.find(component) == m_Handlers_VehicleNotReached.cend()) {
        m_Handlers_VehicleNotReached.insert({component, {}});
    }
    m_Handlers_VehicleNotReached.at(component).push_back(lambda);
}



/**
 * @brief Add handler to be called when a new vehicle is added to the network
 * @param lambda Lambda function whoose parameters are the vehicle ID and node address of new vechile.
 */
void InteropComponent::AddHandler_NewRemoteComponentItem_Generic(const std::function<void(const char* component, int, uint64_t)> &lambda)
{
    m_Handlers_NewRemoteVehicle_Generic.push_back(lambda);
}


/**
 * @brief Add handler to be called when a new vehicle has been removed from the network
 * @param lambda Lambda function whoose parameters are the vehicle ID of removed vechile.
 */
void InteropComponent::AddHandler_RemoteComponentItemRemoved_Generic(const std::function<void(const char* component, int)> &lambda)
{
    m_Handlers_RemoteVehicleRemoved_Generic.push_back(lambda);
}


/**
 * @brief Add handler to be called when tranmission to a vehicle failed for some reason.
 * @param lambda Lambda function to pass vehicle ID and status code
 */
void InteropComponent::AddHandler_ComponentItemTransmitError_Generic(const std::function<void(const char* component, int vehicle, TransmitStatusTypes status)> &lambda)
{
    m_Handlers_VehicleNotReached_Generic.push_back(lambda);
}


/**
 * @brief Send data to a component item
 * @param component Name of component to send to
 * @param destVechileID ID of item
 * @param data Data to send
 * @throws std::runtime_error Thrown if no vehicle of given id is known.
 */
void InteropComponent::SendData(const char* component, const int &destVehicleID, const std::vector<uint8_t> &data)
{
    //address to send to
    uint64_t addr = GetComponent(component)->GetAddr(destVehicleID);

    SendDataToAddress(addr, data, [this, destVehicleID, component](const TransmitStatusTypes &status){

        if(status != TransmitStatusTypes::SUCCESS)
        {
            if(m_Handlers_VehicleNotReached.find(component) != m_Handlers_VehicleNotReached.cend())
            {
                Notify<int, TransmitStatusTypes>(m_Handlers_VehicleNotReached.at(component), destVehicleID, status);
            }
            else {
                Notify<const char*, int, TransmitStatusTypes>(m_Handlers_VehicleNotReached_Generic, component, destVehicleID, status);
            }
        }
    });
}



void InteropComponent::onNewRemoteComponentItem(const char* name, int ID, uint64_t addr)
{
    if(GetComponent(name)->AddExternalItem(ID, addr) == true)
    {
        if(m_Handlers_NewRemoteVehicle.find(name) != m_Handlers_NewRemoteVehicle.cend())
        {
            Notify<int, uint64_t>(m_Handlers_NewRemoteVehicle.at(name), ID, addr);
        }
        else {
            Notify<const char*, int, uint64_t>(m_Handlers_NewRemoteVehicle_Generic, name, ID, addr);
        }
    }
}

void InteropComponent::onRemovedRemoteComponentItem(const char* name, int ID)
{
    GetComponent(name)->RemoveExternalItem(ID);
    if(m_Handlers_RemoteVehicleRemoved.find(name) != m_Handlers_RemoteVehicleRemoved.cend())
    {
        Notify<int>(m_Handlers_RemoteVehicleRemoved.at(name), ID);
    }
    else {
        Notify<const char*, int>(m_Handlers_RemoteVehicleRemoved_Generic, name, ID);
    }
}

std::vector<int> InteropComponent::RetrieveComponentItems(const char* name)
{
    return GetComponent(name)->ContainedIDs();
}
