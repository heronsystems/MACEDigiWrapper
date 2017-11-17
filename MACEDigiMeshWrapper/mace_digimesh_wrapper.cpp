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
    m_ScanThread([this](){run_scan();}),
    m_ShutdownScanThread(false)
{
    m_Radio.SetATParameterAsync<ATData::Integer<uint8_t>>("AP", ATData::Integer<uint8_t>(1), [this](){
        m_NIMutex.lock();
        m_Radio.SetATParameterAsync<ATData::String>("NI", "", [this](){m_NIMutex.unlock();});
    });
}


MACEDigiMeshWrapper::~MACEDigiMeshWrapper() {
    m_Radio.SetATParameterAsync<ATData::String>("AP", "");
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

    m_NIMutex.lock();
    m_Radio.GetATParameterAsync<ATData::String, ShutdownFirstResponse>("NI", [this, ID](const std::vector<ATData::String> &currNIarr) {
       ATData::String currNI = currNIarr.at(0) ;
       bool changeMade = false;
       if(currNI == "") {
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


void MACEDigiMeshWrapper::AddHandler_NewRemoteVehicle(const std::function<void(int, uint64_t)> &lambda)
{
    m_Handlers_NewRemoteVehicle.push_back(lambda);
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

                //if the vehicle of given id doesn't exists, add it
                if(m_VehicleIDToRadioAddr.find(num) == m_VehicleIDToRadioAddr.end()) {
                    m_VehicleIDMutex.lock();
                    m_VehicleIDToRadioAddr.insert({num, node.addr});
                    m_VehicleIDMutex.unlock();

                    Notify<int, uint64_t>(m_Handlers_NewRemoteVehicle, num, node.addr);
                }
            }

        }
    }
}
