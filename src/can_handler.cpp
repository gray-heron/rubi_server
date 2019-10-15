
#include <memory>

#include "board.h"
#include "communication.h"
#include "exceptions.h"
#include "frontend.h"
#include "protocol_defs.h"
#include "types.h" 

#include <thread>

using std::get;
using std::string;

CanHandler::CanHandler(std::string can_name)
{
    // std::vector<uint8_t> lottery_invitation = {RUBI_MSG_COMMAND,
    // RUBI_COMMAND_LOTERRY};
    std::vector<uint8_t> lottery_invitation = {RUBI_MSG_COMMAND,
                                               RUBI_COMMAND_REBOOT};

    max_boards_count = RUBI_ADDRESS_RANGE1_HIGH - RUBI_ADDRESS_RANGE1_LOW;
    address_pool.resize(max_boards_count);
    traffic.resize(max_boards_count);

    socketcan = std::unique_ptr<SocketCan>(new SocketCan(can_name));
    socketcan->Send(std::pair<uint16_t, std::vector<uint8_t>>(
        RUBI_BROADCAST1, lottery_invitation));
}

uint8_t CanHandler::NewBoard(uint16_t lottery_id)
{
    for (unsigned int i = 0; i < address_pool.size(); i++)
    {
        if (!address_pool[i].is_initialized())
        {
            address_pool[i] =
                std::make_shared<BoardCommunicationHandler>(this, i);
            socketcan->Send(std::pair<uint16_t, std::vector<uint8_t>>(
                RUBI_LOTTERY_RANGE_LOW + lottery_id,
                std::vector<uint8_t>({(uint8_t)i})));

            return i;
        }
    }

    ASSERT(false);
    return 0;
}

void CanHandler::Tick(std::chrono::system_clock::time_point time)
{
    if (std::chrono::duration_cast<std::chrono::seconds>(time - last_keepalive)
            .count() >= 1)
    {
        for (const auto &handler : address_pool)
        {
            if (handler && !(*handler)->IsDead() &&
                (*handler)->GetBoard().descriptor)
            {
                (*handler)->KeepAliveRequest();
            }
        }
        last_keepalive = time;
    }

    while (auto msg = socketcan->Receive(1))
    {
        auto rx = *msg;

        if (std::get<0>(rx) >= RUBI_LOTTERY_RANGE_LOW &&
            std::get<0>(rx) <= RUBI_LOTTERY_RANGE_HIGH)
        {
            uint16_t version_reported =
                *(reinterpret_cast<uint16_t *>(std::get<1>(rx).data() + 2));
            if (std::get<1>(rx).size() == 4 &&
                version_reported == RUBI_PROTOCOL_VERSION)
                NewBoard(std::get<0>(rx) - RUBI_LOTTERY_RANGE_LOW);
            else
                log.Error("Board with outdated/incompatible  protocol version "
                          "found on the bus!");
        }
        else if (std::get<0>(rx) >= RUBI_ADDRESS_RANGE1_LOW &&
                 std::get<0>(rx) <= RUBI_ADDRESS_RANGE1_HIGH)
        {
            int id = std::get<0>(rx) - RUBI_ADDRESS_RANGE1_LOW;

            ASSERT(std::get<1>(rx).size() >= 1);
            ASSERT(address_pool[id] != boost::none);

            if ((*address_pool[id])->IsDead())
            {
                log.Error("Received message from board that should be dead!");
                continue;
            }

            switch (std::get<1>(rx)[0] & RUBI_MSG_MASK)
            {
            case RUBI_MSG_LOTTERY:
                (*address_pool[id])->ConfirmAddress();
                break;
            case RUBI_MSG_FIELD:
            case RUBI_MSG_FUNCTION:
            case RUBI_MSG_INFO:
            case RUBI_MSG_BLOCK:
            case RUBI_MSG_EVENT:
            case RUBI_MSG_COMMAND:
                (*address_pool[id])
                    ->protocol->InboundWrapper(
                        std::pair<uint16_t, bytes_t>(get<0>(rx), get<1>(rx)));
                break;

            case RUBI_MSG_INIT_COMPLETE:
                BoardManager::inst().RegisterNewHandler(*address_pool[id]);
                break;
            default:
                assert(0);
            }
        }
        else
        {
            log.Warning("Unknown message received.");
        }
    }
}

uint64_t CanHandler::GetTrafficSoFar(bool reset)
{
    uint64_t total_data =
        socketcan->GetTotalTransmittedDataSize() + socketcan->GetTotalReceivedDataSize();
    uint64_t to_return = total_data - traffic_reported;

    if (reset)
        traffic_reported = total_data;

    return to_return;
}