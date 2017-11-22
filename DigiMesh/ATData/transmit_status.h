#ifndef TRANSMIT_STATUS_H
#define TRANSMIT_STATUS_H

#include "I_AT_data.h"
#include <string>

namespace ATData
{

class TransmitStatus : public IATData {

public:

    TransmitStatus()
    {

    }

    TransmitStatus(const std::vector<uint8_t> &data)
    {
        DeSerialize(data);
    }

    enum StatusTypes
    {
        SUCCESS = 0x00,
        MAC_ACK_FAILURE = 0x01,
        COLLISION_AVOIDANCE_FAILURE = 0x02,
        NETWORK_ACK_FAILURE = 0x21,
        ROUTE_NOT_FOUND = 0x25,
        INTERNAL_RESOURCE_ERROR = 0x31,
        INTERNAL_ERROR = 0x32,
        PAYLOAD_TOO_LARGE = 0x74,
        INDIRECT_MESSAGE_REQUESTED = 0x75
    };

    enum DiscoveryTypes
    {
        NO_DISCOVERY_OVERHEAD = 0x00,
        ROUTE_DISCOVERY = 0x02
    };

    uint8_t retries;
    StatusTypes status;
    DiscoveryTypes disoveryRequired;

private:

    virtual void DeSerialize(const std::vector<uint8_t> &data){
        retries = data[4];
        status = (StatusTypes)data[5];
        disoveryRequired = (DiscoveryTypes)data[6];
    }

    virtual std::vector<uint8_t> Serialize() const {
        throw "Output Only Data";
    }
};

}

#endif // TRANSMIT_STATUS_H
