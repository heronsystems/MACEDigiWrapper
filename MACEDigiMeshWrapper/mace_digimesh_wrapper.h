#ifndef MACE_WRAPPER_H
#define MACE_WRAPPER_H

#include "macewrapper_global.h"

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <mutex>
#include <thread>

#include "digi_mesh_baud_rates.h"
#include "transmit_status_types.h"
#include "component.h"

#include "interop_component.h"


template<const char*... A>
class _MACEDigiMeshWrapper;


template<const char* Type, const char*... Rest>
class _MACEDigiMeshWrapper<Type, Rest...> : public _MACEDigiMeshWrapper<Rest...>
{
private:

public:

    _MACEDigiMeshWrapper<Type, Rest...>()
    {
        _MACEDigiMeshWrapper<Rest...>::m_ElementMap.insert({Type, new Component()});
    }

    ~_MACEDigiMeshWrapper<Type, Rest...>()
    {

    }

};

template <>
class MACEWRAPPERSHARED_EXPORT _MACEDigiMeshWrapper<>
{
public:
    std::unordered_map<std::string, Component*> m_ElementMap;

public:

    _MACEDigiMeshWrapper<>() {

    }
};

template <const char*... A>
class MACEWRAPPERSHARED_EXPORT MACEDigiMeshWrapper : public InteropComponent, public _MACEDigiMeshWrapper<A...>
{
    using _MACEDigiMeshWrapper<A...>::m_ElementMap;

public:


    MACEDigiMeshWrapper(const std::string &port, DigiMeshBaudRates rate, const std::string &nameOfNode = "", bool scanForNodes = false) :
        InteropComponent(port, rate, nameOfNode, scanForNodes)
    {


        //broadcast a request for everyone to send their Elements
        variadicExpand<A...>([this](const char* element) {

            InteropComponent::RequestContainedVehicles(element);
        });

    }


    template <const char* T>
    void AddComponentItem(const int ID)
    {
        InteropComponent::AddComponentItem(T, ID);
    }

    template <const char* T>
    void RemoveComponentItem(const int ID)
    {
        InteropComponent::RemoveComponentItem(T, ID);
    }


    /**
     * @brief Add handler to be called when a new vehicle is added to the network
     * @param lambda Lambda function whoose parameters are the vehicle ID and node address of new vechile.
     */
    template <const char* T>
    void AddHandler_NewRemoteComponentItem(const std::function<void(int, uint64_t)> &lambda)
    {
        InteropComponent::AddHandler_NewRemoteComponentItem(T, lambda);
    }


    /**
     * @brief Add handler to be called when a new vehicle has been removed from the network
     * @param lambda Lambda function whoose parameters are the vehicle ID of removed vechile.
     */
    template <const char* T>
    void AddHandler_RemovedRemoteComponentItem(const std::function<void(int)> &lambda)
    {
        InteropComponent::AddHandler_RemoteComponentItemRemoved(T, lambda);
    }


    /**
     * @brief Add handler to be called when tranmission to a vehicle failed for some reason.
     * @param lambda Lambda function to pass vehicle ID and status code
     */
    template <const char* T>
    void AddHandler_ComponentItemTransmitError(const std::function<void(int vehicle, TransmitStatusTypes status)> &lambda)
    {
        InteropComponent::AddHandler_ComponentItemTransmitError(T, lambda);
    }


    template <const char* T>
    SendData(const int &destVehicleID, const std::vector<uint8_t> &data)
    {
        InteropComponent::SendData(T, destVehicleID, data);
    }


private:

    virtual Component* GetComponent(const char* name)
    {
        return m_ElementMap[name];
    }
};


#endif // MACE_WRAPPER_H
