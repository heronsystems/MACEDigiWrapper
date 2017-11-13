#include "mace_digimesh_wrapper.h"

#include "serial_configuration.h"

#include <iostream>


#define START_BYTE 0x7e;
#define BROADCAST_ADDRESS = '000000000000ffff';
// command types
#define FRAME_AT_COMMAND 0x08;
#define FRAME_AT_COMMAND_RESPONSE 0x88;
#define FRAME_REMOTE_AT_COMMAND 0x17;
#define FRAME_REMOTE_AT_COMMAND_RESPONSE 0x97;
#define FRAME_MODEM_STATUS 0x8a;
#define FRAME_TRANSMIT_REQUEST 0x10;
#define FRAME_TRANSMIT_STATUS 0x8b;
#define FRAME_RECEIVE_PACKET 0x90;

#define CALLBACK_QUEUE_SIZE 256

MACEDigiMeshWrapper::MACEDigiMeshWrapper(const std::string &commPort, const DigiMeshBaudRates &baudRate) :
    m_Link(NULL)
{
    m_CurrentFrames = new Frame[CALLBACK_QUEUE_SIZE];
    for(int i = 0 ; i < CALLBACK_QUEUE_SIZE ; i++) {
        m_CurrentFrames[i].inUse = false;
        m_CurrentFrames[i].callbackEnabled = false;
    }
    m_PreviousFrame = 0;

    SerialConfiguration config;
    config.setBaud(baudRate);
    config.setPortName(commPort);
    config.setDataBits(8);
    config.setParity(QSerialPort::NoParity);
    config.setStopBits(1);
    config.setFlowControl(QSerialPort::NoFlowControl);

    m_Link = new SerialLink(config);
    m_Link->Connect();
}

MACEDigiMeshWrapper::~MACEDigiMeshWrapper() {
    delete[] m_CurrentFrames;
}

/**
 * @brief SetOnNewVehicleCallback
 * Set lambda to be called when a new vehicle is discovered by DigiMesh
 * @param func lambda to call.
 */
void MACEDigiMeshWrapper::SetOnNewVehicleCallback(std::function<void(const int)> func)
{
    this->m_NewVehicleCallback = func;
}


/**
 * @brief SetNewDataCallback
 * Set callback to be notified when new data has been transmitted to this node
 * @param func Function to call upon new data
 */
void MACEDigiMeshWrapper::SetNewDataCallback(std::function<void(const std::vector<uint8_t> &)> func)
{
    this->m_NewDataCallback = func;
}


/**
 * @brief AddVehicle
 * Add a vehicle to the DigiMesh network.
 * @param ID Unique Identifier of vehicle
 */
void MACEDigiMeshWrapper::AddVehicle(const int ID)
{

}


/**
 * @brief BroadcastData
 * Broadcast data to all nodes on network
 * @param data
 */
void MACEDigiMeshWrapper::BroadcastData(const std::vector<uint8_t> &data)
{

}


/**
 * @brief SendData
 * Send data to a specific vehicle
 * @param vechileID
 * @param data
 */
void MACEDigiMeshWrapper::SendData(const int vechileID, const std::vector<uint8_t> &data)
{

}

int calc_checksum(const char *buf, const size_t start, const size_t end) {
    size_t sum = 0;
    for (size_t i = start; i < end; i++) {
        sum += buf[i];
    }
    return (0xff - sum) & 0xff;
}

void MACEDigiMeshWrapper::GetParameterAsync(const std::string &parameterName, const std::function<void(const std::string &)> &callback, const std::vector<char> &data)
{
    int frame_id = reserve_next_frame_id();
    if(frame_id == -1) {
        throw std::runtime_error("Queue Full");
    }

    size_t param_len = data.size();

    char *tx_buf = new char[8 + param_len];
    tx_buf[0] = START_BYTE;
    tx_buf[1] = (0x04 + param_len) >> 8;
    tx_buf[2] = (0x04 + param_len) & 0xff;
    tx_buf[3] = FRAME_AT_COMMAND;
    tx_buf[4] = frame_id;
    tx_buf[5] = parameterName[0];
    tx_buf[6] = parameterName[1];

    // if we have a parameter, copy it over
    if (param_len > 0) {
        for(size_t i = 0 ; i < param_len ; i++) {
            tx_buf[7 + i] = data[i];
        }
    }

    tx_buf[7 + param_len] = calc_checksum(tx_buf, 3, 8 + param_len-1);

    for(int i = 0 ; i < 8 + param_len ; i++) {
        printf("  %d 0x%x %c\n", tx_buf[i], tx_buf[i], tx_buf[i]);
    }

    //console.log(tx_buf.toString('hex').replace(/(.{2})/g, "$1 "));
    m_Link->MarshalOnThread([this, tx_buf, param_len, frame_id, callback](){
        m_Link->WriteBytes(tx_buf, 8 + param_len);

        m_CurrentFrames[frame_id].callback = callback;
        m_CurrentFrames[frame_id].callbackEnabled = true;

        delete[] tx_buf;
    });
}

void MACEDigiMeshWrapper::SetParameter(const std::string &parameterName, const std::string &value) const
{

}

void MACEDigiMeshWrapper::ReceiveData(SerialLink *link_ptr, const std::vector<uint8_t> &buffer) const
{
    std::cout << "Receive Data" << std::endl;
}

void MACEDigiMeshWrapper::CommunicationError(const SerialLink* link_ptr, const std::string &type, const std::string &msg) const
{

}

void MACEDigiMeshWrapper::CommunicationUpdate(const SerialLink *link_ptr, const std::string &name, const std::string &msg) const
{

}

void MACEDigiMeshWrapper::Connected(const SerialLink* link_ptr) const
{

}

void MACEDigiMeshWrapper::ConnectionRemoved(const SerialLink *link_ptr) const
{

}


int MACEDigiMeshWrapper::reserve_next_frame_id()
{
    std::lock_guard<std::mutex> lock(m_FrameSelectionMutex);

    // save last id
    int framePrediction = m_PreviousFrame;
    // advance until we find an empty slot
    do {
        framePrediction++;

        //wrap around
        if (framePrediction > CALLBACK_QUEUE_SIZE-1){
            framePrediction = 0;
        }

        // if we've gone all the way around, then we can't send
        if (framePrediction == m_PreviousFrame)
        {
            return -1;
        }
    }
    while (m_CurrentFrames[framePrediction].inUse == true);

    m_CurrentFrames[framePrediction].inUse = true;
    m_PreviousFrame = framePrediction;

    return framePrediction;
}
