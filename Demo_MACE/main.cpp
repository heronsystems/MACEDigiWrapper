#include <iostream>

#include "mace_digimesh_wrapper.h"

int main(int argc, char *argv[])
{
    const char* RADIO1 = "COM6";

    MACEDigiMeshWrapper wrapper(RADIO1, DigiMeshBaudRates::Baud9600);

    wrapper.AddHandler_NewRemoteVehicle([](int num, uint64_t addr){
        printf("New Remote Vehicle\n");
        printf("  Vehicle ID:    %d\n", num);
        printf("  DigiMesh Addr: %llx", addr);
    });

    wrapper.AddHandler_Data([](const std::vector<uint8_t> &data) {
        printf("New Data Received");
    });

    wrapper.AddVehicle(1);

    wrapper.SendData(1, {0, 1, 2, 3});


    while(true) {

    }
}
