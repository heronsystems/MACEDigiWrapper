#include <iostream>

#include "digimesh_radio.h"
#include "digi_mesh_baud_rates.h"

#include "ATData/index.h"

#include "frame-persistance/types/index.h"

int main(int argc, char *argv[])
{
    std::cout << "Hello World!" << std::endl;

    const char* RADIO1 = "COM6";
    const char* RADIO2 = "COM4";

    DigiMeshRadio wrapper1(RADIO1, DigiMeshBaudRates::Baud9600);
    DigiMeshRadio wrapper2(RADIO2, DigiMeshBaudRates::Baud9600);



    wrapper2.AddMessageHandler([RADIO2](const ATData::Message &data)
    {
        printf("%s received message:\n", RADIO2);
        printf("  From: %llx\n", data.addr);
        printf("  Broadcast: %s\n", data.broadcast?"true":"false");
        printf("  Data:\n", data.broadcast);
        for(size_t i = 0 ; i < data.data.size() ; i++) {
            printf("      %c %3d %2x\n", data.data[i], data.data[i], data.data[i]);
        }
    });

    wrapper1.SetATParameterAsync<ATData::Integer<uint8_t>>("AP", ATData::Integer<uint8_t>(1));
    //wrapper2.SetATParameterAsync<ATData::Integer<uint8_t>>("AP", ATData::Integer<uint8_t>(1));



    wrapper1.SetATParameterAsync<ATData::String>("NI", RADIO1);
    wrapper1.GetATParameterAsync<ATData::String>("NI", [RADIO1](const std::vector<ATData::String> &a){
        if(a.size() > 0) {
            printf("%s Ni: %s\n", a[0].c_str(), RADIO1);
        }
    }, ShutdownFirstResponse());


    /*
    wrapper2.SetATParameterAsync<ATData::String>("NI", "BB");
    wrapper2.GetATParameterAsync<ATData::String>("NI", [](const std::vector<ATData::String> &a){
        if(a.size() > 0) {
            printf("COM5 Ni: %s\n", a[0].c_str());
        }
    }, ShutdownFirstResponse());



    wrapper2.GetATParameterAsync<ATData::NodeDiscovery>("ND", [](const std::vector<ATData::NodeDiscovery> &a){
        printf("Node Discovery done!\n");
        for(size_t i = 0 ; i < a.size() ; i++) {
            ATData::NodeDiscovery node = a.at(i);
            printf("Node: %s\n", node.NI.c_str());
            printf("  addr:            0x%llx\n", node.addr);
            printf("  device type:     0x%x\n", node.device_type);
            printf("  network addr:    0x%x\n", node.network_addr);
            printf("  parent net addr: 0x%x\n", node.parent_net_addr);
            printf("  manufacturer id: 0x%x\n", node.manufacturer_id);
            printf("  profile id:      0x%x\n", node.profile_id);
        }
    }, CollectAfterTimeout(15000));
    */

    auto message = (ATData::String("Hi")).Serialize();
    wrapper1.SendMessage(message);

    while(true) {

    }
}
