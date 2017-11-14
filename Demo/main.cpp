#include <iostream>

#include "mace_digimesh_wrapper.h"
#include "digi_mesh_baud_rates.h"

int main(int argc, char *argv[])
{
    std::cout << "Hello World!" << std::endl;

    MACEDigiMeshWrapper wrapper("COM4", DigiMeshBaudRates::Baud9600);

    /*
    wrapper.SetATParameterAsync<std::string>("NI", "B");

    ResponseBack response;
    wrapper.GetATParameterAsync<std::string>("NI", [](const std::vector<std::string> &a){
       printf("Ni Return: %s\n", a[0].c_str());
    }, response);
    */




    CollectAfterTimeout timeout(15000);
    wrapper.GetATParameterAsync<std::string>("ND", [](const std::vector<std::string> &a){
        printf("Node Discovery done!\n");
    }, timeout);


    while(true) {

    }
}
