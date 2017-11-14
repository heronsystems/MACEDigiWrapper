#include "mace_digimesh_wrapper.h"

#include "serial_configuration.h"

#include <iostream>


#define START_BYTE 0x7e
#define BROADCAST_ADDRESS = '000000000000ffff'
// command types
#define FRAME_AT_COMMAND 0x08
#define FRAME_AT_COMMAND_RESPONSE 0x88
#define FRAME_REMOTE_AT_COMMAND 0x17
#define FRAME_REMOTE_AT_COMMAND_RESPONSE 0x97
#define FRAME_MODEM_STATUS 0x8a
#define FRAME_TRANSMIT_REQUEST 0x10
#define FRAME_TRANSMIT_STATUS 0x8b
#define FRAME_RECEIVE_PACKET 0x90

#define CALLBACK_QUEUE_SIZE 256

MACEDigiMeshWrapper::MACEDigiMeshWrapper(const std::string &commPort, const DigiMeshBaudRates &baudRate) :
    m_Link(NULL)
{
    m_CurrentFrames = new Frame[CALLBACK_QUEUE_SIZE];
    for(int i = 0 ; i < CALLBACK_QUEUE_SIZE ; i++) {
        m_CurrentFrames[i].inUse = false;
        m_CurrentFrames[i].callback = NULL;
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

    m_Link->AddListener(this);
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


void MACEDigiMeshWrapper::ReceiveData(SerialLink *link_ptr, const std::vector<uint8_t> &buffer)
{
    //add what we received to the current buffer.
    for(size_t i = 0 ; i < buffer.size() ; i++) {
        m_CurrBuf.push_back(buffer.at(i));
    }


    //start infinite loop to pick up multiple packets sent at same time
    while(true) {

        //if there aren't three bytes received we have to wait more
        if(m_CurrBuf.size() < 3) {
            break;
        }

        // add 4 bytes for start, length, and checksum
        uint8_t packet_length = (m_CurrBuf[1]<<8 |m_CurrBuf[2]) + 4;

        //if we have received part of the packet, but not entire thing then we need to wait longer
        if(m_CurrBuf.size() < packet_length) {
            return;
        }

        //splice m_CurrBuff to just our packet we care about.
        std::vector<uint8_t> packet(
            std::make_move_iterator(m_CurrBuf.begin() + 3),
            std::make_move_iterator(m_CurrBuf.begin() + packet_length));
        m_CurrBuf.erase(m_CurrBuf.begin(), m_CurrBuf.begin() + packet_length);

        switch(packet[0])
        {
            case FRAME_AT_COMMAND_RESPONSE:
                handle_AT_command_response(packet);
                break;
            default:
                throw std::runtime_error("unknown packet type received: " + packet[0]);
        }



    }
}

void MACEDigiMeshWrapper::CommunicationError(const SerialLink* link_ptr, const std::string &type, const std::string &msg)
{

}

void MACEDigiMeshWrapper::CommunicationUpdate(const SerialLink *link_ptr, const std::string &name, const std::string &msg)
{

}

void MACEDigiMeshWrapper::Connected(const SerialLink* link_ptr)
{

}

void MACEDigiMeshWrapper::ConnectionRemoved(const SerialLink *link_ptr)
{

}


int MACEDigiMeshWrapper::AT_command_helper(const std::string &parameterName, const std::shared_ptr<Callback<std::string>> &callback, const std::vector<char> &data) {
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

    //console.log(tx_buf.toString('hex').replace(/(.{2})/g, "$1 "));
    m_Link->MarshalOnThread([this, tx_buf, param_len, frame_id, callback](){
        m_CurrentFrames[frame_id].callback = callback;
        m_Link->WriteBytes(tx_buf, 8 + param_len);

        delete[] tx_buf;
    });

    return frame_id;

}

void MACEDigiMeshWrapper::handle_AT_command_response(const std::vector<uint8_t> &buf) {
    uint8_t frame_id = buf[1];

    uint8_t status = buf[4];
    if(status) {
        //error
    }

    std::string commandRespondingTo = "";
    commandRespondingTo.push_back(buf[2]);
    commandRespondingTo.push_back(buf[3]);

    std::string str = "";
    for(size_t i = 5 ; i < buf.size()-1 ; i++) {
        str += buf[i];
    }
    find_and_invokve_frame<std::string>(frame_id, str);

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


void MACEDigiMeshWrapper::finish_frame(int frame_id)
{
    this->m_CurrentFrames[frame_id].inUse = false;
}
