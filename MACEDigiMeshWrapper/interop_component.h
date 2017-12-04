#ifndef INTEROP_ENTITIES_H
#define INTEROP_ENTITIES_H

#include "interop.h"
#include "component.h"

#include "macewrapper_global.h"

class InteropComponent : public Interop
{
private:


    std::map<std::string, std::vector<std::function<void(int, uint64_t)>>> m_Handlers_NewRemoteVehicle;
    std::map<std::string, std::vector<std::function<void(int)>>> m_Handlers_RemoteVehicleRemoved;
    std::map<std::string, std::vector<std::function<void(int, TransmitStatusTypes)>>> m_Handlers_VehicleNotReached;


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
    InteropComponent(const std::string &port, DigiMeshBaudRates rate, const std::string &nameOfNode = "", bool scanForNodes = false);

protected:

    void AddComponentItem(const char* component, const int ID);


    void RemoveComponentItem(const char* component, const int ID);


    /**
     * @brief Add handler to be called when a new vehicle is added to the network
     * @param lambda Lambda function whoose parameters are the vehicle ID and node address of new vechile.
     */
    void AddHandler_NewRemoteVehicle(const char* component, const std::function<void(int, uint64_t)> &lambda);


    /**
     * @brief Add handler to be called when a new vehicle has been removed from the network
     * @param lambda Lambda function whoose parameters are the vehicle ID of removed vechile.
     */
    void AddHandler_RemoteVehicleRemoved(const char* component, const std::function<void(int)> &lambda);


    /**
     * @brief Add handler to be called when tranmission to a vehicle failed for some reason.
     * @param lambda Lambda function to pass vehicle ID and status code
     */
    void AddHandler_VehicleTransmitError(const char* component, const std::function<void(int vehicle, TransmitStatusTypes status)> &lambda);




protected:

    /**
     * @brief Send data to a component item
     * @param component Name of component to send to
     * @param destVechileID ID of item
     * @param data Data to send
     * @throws std::runtime_error Thrown if no vehicle of given id is known.
     */
    void SendData(const char* component, const int &destVehicleID, const std::vector<uint8_t> &data);

protected:


    virtual void onNewRemoteComponentItem(const char* name, int ID, uint64_t addr);

    virtual void onRemovedRemoteComponentItem(const char* name, int ID);

    virtual std::vector<int> RetrieveComponentItems(const char* name);


protected:


    virtual Component* GetComponent(const char*) = 0;

};

#endif // INTEROP_ENTITIES_H
