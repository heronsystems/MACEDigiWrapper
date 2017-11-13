#include <iostream>

#include "mace_digimesh_wrapper.h"
#include "digi_mesh_baud_rates.h"

int main(int argc, char *argv[])
{
    std::cout << "Hello World!" << std::endl;

    MACEDigiMeshWrapper wrapper("COM4", DigiMeshBaudRates::Baud9600);

    wrapper.GetParameterAsync("NI", [](const std::string &a){});

    while(true) {

    }
}
