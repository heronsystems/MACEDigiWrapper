#ifndef MACE_DIGIMESH_WRAPPER_H
#define MACE_DIGIMESH_WRAPPER_H

#include "macedigimeshwrapper_global.h"
#include <vector>
#include <map>
#include <functional>
#include "digi_mesh_baud_rates.h"

#include "serial_link.h"

#include "i_link_events.h"

#include "frame-persistance/behaviors/index.h"

#include "ATData/index.h"

#include "math_helper.h"
#include "callback.h"


#define START_BYTE 0x7e
#define BROADCAST_ADDRESS 0x000000000000ffff
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


class MACEDIGIMESHWRAPPERSHARED_EXPORT MACEDigiMeshWrapper : private ILinkEvents
{
private:

    struct Frame{
        std::shared_ptr<FramePersistanceBehavior<>> framePersistance;
        bool inUse;
    };

    SerialLink *m_Link;

    std::vector<int> m_OwnVehicles;
    std::map<int, int> m_RemoteVehiclesToAddress;

    std::function<void(const int)> m_NewVehicleCallback;
    std::function<void(const std::vector<uint8_t> &)> m_NewDataCallback;

    Frame *m_CurrentFrames;
    int m_PreviousFrame;
    std::mutex m_FrameSelectionMutex;

    std::vector<uint8_t> m_CurrBuf;

    std::vector<std::function<void(const ATData::Message&)>> m_MessageHandlers;

public:
    MACEDigiMeshWrapper(const std::string &commPort, const DigiMeshBaudRates &baudRate);

    ~MACEDigiMeshWrapper();

    /**
     * @brief SetOnNewVehicleCallback
     * Set lambda to be called when a new vehicle is discovered by DigiMesh
     * @param func lambda to call.
     */
    void SetOnNewVehicleCallback(std::function<void(const int)> func);


    /**
     * @brief SetNewDataCallback
     * Set callback to be notified when new data has been transmitted to this node
     * @param func Function to call upon new data
     */
    void SetNewDataCallback(std::function<void(const std::vector<uint8_t> &)> func);

    void AddMessageHandler(const std::function<void(const ATData::Message&)> &lambda)
    {
        m_MessageHandlers.push_back(lambda);
    }


    /**
     * @brief AddVehicle
     * Add a vehicle to the DigiMesh network.
     * @param ID Unique Identifier of vehicle
     */
    void AddVehicle(const int ID);


    /**
     * @brief BroadcastData
     * Broadcast data to all nodes on network
     * @param data
     */
    void BroadcastData(const std::vector<uint8_t> &data);


    /**
     * @brief SendData
     * Send data to a specific vehicle
     * @param vechileID
     * @param data
     */
    void SendData(const int vechileID, const std::vector<uint8_t> &data);


    template <typename T, typename P>
    void GetATParameterAsync(const std::string &parameterName, const std::function<void(const std::vector<T> &)> &callback, const P &persistance)
    {
        static_assert(std::is_base_of<ATData::IATData, T>::value, "T must be a descendant of ATDATA::IATDATA");

        std::shared_ptr<FramePersistanceBehavior<P>> frameBehavior = std::make_shared<FramePersistanceBehavior<P>>(persistance);
        ((FramePersistanceBehavior<>*)frameBehavior.get())->setCallback<T>(callback);

        int frame_id = AT_command_helper(parameterName, frameBehavior);
    }


    void GetATParameterSync(const std::string &parameterName)
    {
        throw std::runtime_error("Not Implimented");
    }


    template <typename T>
    void SetATParameterAsync(const std::string &parameterName, const T &value)
    {
        static_assert(std::is_base_of<ATData::IATData, T>::value, "T must be a descendant of ATDATA::IATDATA");

        SetATParameterAsync<T, ATData::Void>(parameterName, value, [](const std::vector<ATData::Void>&){});
    }


    template <typename T, typename RT>
    void SetATParameterAsync(const std::string &parameterName, const T &value, const std::function<void(const std::vector<RT> &)> &callback)
    {
        static_assert(std::is_base_of<ATData::IATData, T>::value, "T must be a descendant of ATDATA::IATDATA");

        std::shared_ptr<FramePersistanceBehavior<ShutdownFirstResponse>> frameBehavior = std::make_shared<FramePersistanceBehavior<ShutdownFirstResponse>>(ShutdownFirstResponse());
        ((FramePersistanceBehavior<>*)frameBehavior.get())->setCallback<RT>(callback);

        std::vector<uint8_t> data = value.Serialize();

        AT_command_helper(parameterName, frameBehavior, data);
    }

    void SendMessage(const std::vector<uint8_t> &data)
    {
        SendMessage(data, BROADCAST_ADDRESS);
    }

    void SendMessage(const std::vector<uint8_t> &data, const uint64_t &addr)
    {
        int packet_length = 14 + data.size();
        int total_length = packet_length+4;
        char *tx_buf = new char[total_length];
        int frame_id = reserve_next_frame_id();

        tx_buf[0] = START_BYTE;
        tx_buf[1] = (packet_length >> 8) & 0xFF;
        tx_buf[2] = packet_length & 0xFF;
        tx_buf[3] = FRAME_TRANSMIT_REQUEST;
        tx_buf[4] = frame_id;
        for(size_t i = 0 ; i < 8 ; i++) {
            uint64_t a = (addr & (0xFFll << (8*(7-i)))) >> (8*(7-i));
            tx_buf[5+i] = (char)a;
        }
        tx_buf[13] = 0xFF;
        tx_buf[14] = 0xFe;
        tx_buf[15] = 0x00;
        tx_buf[16] = 0x00;
        for(size_t i = 0 ; i < data.size() ; i++) {
            tx_buf[17+i] = data.at(i);
        }
        tx_buf[total_length-1] = MathHelper::calc_checksum(tx_buf, 3, total_length-2);
        m_Link->MarshalOnThread([this, tx_buf, total_length, frame_id](){
            m_Link->WriteBytes(tx_buf, total_length);

            delete[] tx_buf;
        });
    }



private:

    virtual void ReceiveData(SerialLink *link_ptr, const std::vector<uint8_t> &buffer);

    virtual void CommunicationError(const SerialLink* link_ptr, const std::string &type, const std::string &msg);

    virtual void CommunicationUpdate(const SerialLink *link_ptr, const std::string &name, const std::string &msg);

    virtual void Connected(const SerialLink* link_ptr);

    virtual void ConnectionRemoved(const SerialLink *link_ptr);

private:

    int AT_command_helper(const std::string &parameterName, const std::shared_ptr<FramePersistanceBehavior<>> &frameBehavior, const std::vector<uint8_t> &data = {})
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

        tx_buf[7 + param_len] = MathHelper::calc_checksum(tx_buf, 3, 8 + param_len-1);

        //console.log(tx_buf.toString('hex').replace(/(.{2})/g, "$1 "));
        m_Link->MarshalOnThread([this, tx_buf, param_len, frame_id, frameBehavior](){
            m_CurrentFrames[frame_id].framePersistance = frameBehavior;
            m_Link->WriteBytes(tx_buf, 8 + param_len);

            delete[] tx_buf;
        });

        return frame_id;
    }

    void handle_AT_command_response(const std::vector<uint8_t> &buff);

    void handle_receive_packet(const std::vector<uint8_t> &);

    int reserve_next_frame_id();

    void find_and_invokve_frame(int frame_id, const std::vector<uint8_t> &data)
    {
        if(this->m_CurrentFrames[frame_id].framePersistance->HasCallback() == true) {
            this->m_CurrentFrames[frame_id].framePersistance->AddFrameReturn(frame_id, data);
        }
    }


    void finish_frame(int frame_id);


};

#endif // MACE_DIGIMESH_WRAPPER_H
