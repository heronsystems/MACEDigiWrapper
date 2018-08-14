#include <iostream>

#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

#include "mace_digimesh_wrapper.h"


extern char MACE_INSTANCE[] = "MaceInstance";
extern char VEHICLE[] = "Vehicle";


int main(int argc, char *argv[])
{



    const char* RADIO1 = "/dev/ttyUSB0";
    const char* RADIO2 = "/dev/ttyUSB1";
    const char* RADIO3 = "/dev/ttyUSB2";


    MACEDigiMeshWrapper<VEHICLE,MACE_INSTANCE> *wrapper1 = new MACEDigiMeshWrapper<VEHICLE,MACE_INSTANCE>(RADIO1, DigiMeshBaudRates::Baud9600);
    MACEDigiMeshWrapper<VEHICLE,MACE_INSTANCE> *wrapper2 = new MACEDigiMeshWrapper<VEHICLE,MACE_INSTANCE>(RADIO2, DigiMeshBaudRates::Baud9600);




    wrapper1->AddHandler_NewRemoteComponentItem_Generic([RADIO1](const ResourceKey &resourceKey, const ResourceValue &resourceValue, uint64_t addr){
        printf("%s\n New Remote Resource\n", RADIO1);
        printf("  Resource Name: \n  ");
        for(size_t i = 0 ; i < resourceKey.size() ; i++)
        {
            printf("%s  ", resourceKey.at(i).c_str());
        }
        printf("\n  Resource Value: \n    ");
        for(size_t i = 0 ; i < resourceValue.size() ; i++)
        {
            printf("%d  ", resourceValue.at(i));
        }
        printf("\n  DigiMesh Addr: %llx\n\n", addr);
    });



    wrapper2->AddHandler_NewRemoteComponentItem_Generic([RADIO2](const ResourceKey &resourceKey, const ResourceValue &resourceValue, uint64_t addr){
        printf("%s\n New Remote Resource\n", RADIO2);
        printf("  Resource Name: \n  ");
        for(size_t i = 0 ; i < resourceKey.size() ; i++)
        {
            printf("%s  ", resourceKey.at(i).c_str());
        }
        printf("\n  Resource Value: \n    ");
        for(size_t i = 0 ; i < resourceValue.size() ; i++)
        {
            printf("%d  ", resourceValue.at(i));
        }
        printf("\n  DigiMesh Addr: %llx\n\n", addr);
    });




    wrapper1->AddHandler_Data([RADIO1](const std::vector<uint8_t> &data) {
        printf("%s\n New Data Received\n  Size: %d\n", RADIO1, data.size());
    });


    wrapper2->AddHandler_Data([RADIO2](const std::vector<uint8_t> &data) {
        printf("%s\n New Data Received\n  Size: %d\n", RADIO2, data.size());
    });


    Sleep(2000);

    wrapper1->AddResource<MACE_INSTANCE>(1);
    wrapper2->AddResource<MACE_INSTANCE>(2);
    //wrapper3->AddResource<MACE_INSTANCE>(3);

    //wrapper1->AddResource<MACE_INSTANCE, VEHICLE>(1,1);
    //wrapper2->AddResource<MACE_INSTANCE, VEHICLE>(2,1);
    //wrapper3->AddResource<MACE_INSTANCE, VEHICLE>(3,1);

    Sleep(1000);

    printf("---------------------------------------------------------------\n");

    MACEDigiMeshWrapper<VEHICLE,MACE_INSTANCE> *wrapper3 = new MACEDigiMeshWrapper<VEHICLE,MACE_INSTANCE>(RADIO3, DigiMeshBaudRates::Baud9600);

    wrapper3->AddHandler_NewRemoteComponentItem_Generic([RADIO3](const ResourceKey &resourceKey, const ResourceValue &resourceValue, uint64_t addr){
        printf("%s\n New Remote Resource\n", RADIO3);
        printf("  Resource Name: \n  ");
        for(size_t i = 0 ; i < resourceKey.size() ; i++)
        {
            printf("%s  ", resourceKey.at(i).c_str());
        }
        printf("\n  Resource Value: \n    ");
        for(size_t i = 0 ; i < resourceValue.size() ; i++)
        {
            printf("%d  ", resourceValue.at(i));
        }
        printf("\n  DigiMesh Addr: %llx\n\n", addr);
    });


    wrapper3->AddHandler_Data([RADIO3](const std::vector<uint8_t> &data) {
        printf("%s\n New Data Received. Size: %d\n\n", RADIO3, data.size());
    });


    Sleep(1000);

    printf("---------------------------------------------------------------\n");

    wrapper3->AddResource<MACE_INSTANCE>(3);

    ResourceKey key1;
    ResourceValue value1;
    key1.AddNameToResourceKey(MACE_INSTANCE);
    value1.AddValueToResourceKey(1);
    wrapper2->SendData({0x01}, key1, value1);

    Sleep(1000);

    ResourceKey key2;
    ResourceValue value2;
    key2.AddNameToResourceKey(MACE_INSTANCE);
    value2.AddValueToResourceKey(2);
    wrapper1->SendData({0x01}, key2, value2);


    Sleep(1000);


    std::function<int(int min, int max)> randGenerator = [](int min, int max){
        return rand() % max + min;
    };


    std::thread thread2([wrapper3, randGenerator](){

        while(true)
        {
            std::vector<uint8_t> data;
            int len = randGenerator(2, 100);
            for(int i = 0 ; i < len ; i++)
            {
                data.push_back(len);
            }
            data = {0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07};
            wrapper3->BroadcastData(data);
            Sleep(50);
        }
    });


    std::thread thread3([wrapper3, randGenerator](){

        while(true)
        {
            std::vector<uint8_t> data;
            int len = randGenerator(2, 100);
            for(int i = 0 ; i < len ; i++)
            {
                data.push_back(len);
            }
            data = {0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07};
            wrapper3->BroadcastData(data);
            Sleep(50);
        }
    });




    std::thread thread4([wrapper3, randGenerator](){

        while(true)
        {
            std::vector<uint8_t> data;
            int len = randGenerator(2, 100);
            for(int i = 0 ; i < len ; i++)
            {
                data.push_back(len);
            }
            data = {0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07};
            wrapper3->BroadcastData(data);
            Sleep(50);
        }
    });


    std::thread thread5([wrapper3, randGenerator](){

        while(true)
        {
            std::vector<uint8_t> data;
            int len = randGenerator(2, 100);
            for(int i = 0 ; i < len ; i++)
            {
                data.push_back(len);
            }
            data = {0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07};
            wrapper3->BroadcastData(data);
            Sleep(50);
        }
    });



    thread2.join();
    thread3.join();
    thread4.join();
    thread5.join();

}
