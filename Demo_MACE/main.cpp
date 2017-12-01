#include <iostream>
#include <windows.h>

#include "mace_digimesh_wrapper.h"

int main(int argc, char *argv[])
{
    const char* RADIO1 = "COM4";
    const char* RADIO2 = "COM5";

    MACEDigiMeshWrapper *wrapper1 = new MACEDigiMeshWrapper(RADIO1, DigiMeshBaudRates::Baud9600, "A");
    MACEDigiMeshWrapper *wrapper2 = new MACEDigiMeshWrapper(RADIO2, DigiMeshBaudRates::Baud9600, "B");

    wrapper1->AddHandler_NewRemoteVehicle([RADIO1](int num, uint64_t addr){
        printf("%s\n New Remote Vehicle\n", RADIO1);
        printf("  Vehicle ID:    %d\n", num);
        printf("  DigiMesh Addr: %llx\n\n", addr);
    });

    wrapper2->AddHandler_NewRemoteVehicle([RADIO2](int num, uint64_t addr){
        printf("%s\n New Remote Vehicle\n", RADIO2);
        printf("  Vehicle ID:    %d\n", num);
        printf("  DigiMesh Addr: %llx\n\n", addr);
    });

    wrapper1->AddHandler_Data([RADIO1](const std::vector<uint8_t> &data) {
        printf("%s\n New Data Received\n\n", RADIO1);
    });

    wrapper2->AddHandler_Data([RADIO2](const std::vector<uint8_t> &data) {
        printf("%s\n New Data Received\n\n", RADIO2);
    });

    wrapper1->AddVehicle(1);
    wrapper2->AddVehicle(2);

    Sleep(7000);

    wrapper1->SendData(2, {0x1, 0x2, 0x3});
    wrapper2->BroadcastData({0x1, 0x2, 0x3});

    Sleep(1000);

    //wrapper1->RemoveVehicle(1);
    //wrapper2->RemoveVehicle(2);



    while(true) {

    }
}
