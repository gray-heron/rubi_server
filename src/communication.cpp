#include <memory>

#include "board.h"
#include "commons.h"
#include "communication.h"
#include "exceptions.h"
#include "frontend.h"
#include "protocol_defs.h"

#include <thread>

using std::string;
using std::get;

BoardInstance BoardCommunicationHandler::GetBoard() { return inst; }

bool BoardCommunicationHandler::IsDead() { return dead; }
bool BoardCommunicationHandler::IsLost() { return lost; }

void BoardCommunicationHandler::FFDataOutbound(
    std::shared_ptr<FFDescriptor> desc, std::vector<uint8_t> &data)
{
    protocol->SendFFData(desc->ffid, desc->GetFFType(), data);
}

BoardCommunicationHandler::BoardCommunicationHandler(CanHandler *can_handler,
                                                     uint8_t board_nodeid)
    : can_handler(can_handler), dead(false), addressed(false),
      operational(false), lost(false), keep_alives_missed(0),
      keep_alive_received(true), received_descriptors(0)
{
    protocol = std::unique_ptr<ProtocolHandler>(
        new ProtocolHandler(this, board_nodeid, can_handler));
}

void BoardCommunicationHandler::DescriptionDataInbound(
    int desc_type, std::vector<uint8_t> &data)
{
    std::string value = DataToString(data);
    if (!inst.descriptor)
    {
        ASSERT(desc_type == RUBI_INFO_BOARD_NAME);

        inst.descriptor = std::make_shared<BoardDescriptor>();
    }

    inst.descriptor->ApplyInfo(desc_type, value);
}

void BoardCommunicationHandler::ErrorInbound(int error_type,
                                             std::vector<uint8_t> &data)
{
    string error_msg;

    switch (error_type)
    {
    case RUBI_ERROR_ASSERT:
        error_msg = "An assertion has fired on board " + (std::string)inst +
                    " in file ";

        for (unsigned int i = 4; i < data.size(); i++)
            error_msg += (char)data[i];

        error_msg += std::string(" at line ") +
                     std::to_string(*((int32_t *)data.data()));

        break;
    default:
        error_msg = "An unknown error has occured on board " + (string)inst;
    }

    BoardManager::inst().frontend->LogError(error_msg);
}

void BoardCommunicationHandler::CommandInbound(int command_id,
                                               std::vector<uint8_t> &data)
{
    ASSERT(command_id == RUBI_COMMAND_KEEPALIVE);
    // TODO keep-alive id

    keep_alive_received = true;
    lost = false;
    keep_alives_missed = 0;
}

void BoardCommunicationHandler::KeepAliveRequest()
{
    if (!keep_alive_received)
    {
        keep_alives_missed += 1;
        lost = true;

        BoardManager::inst().frontend->LogWarning(
            "Didn't receive keep-alive from " + (string)inst + "!");

        if (keep_alives_missed == 5)
        {
            dead = true;
            BoardManager::inst().frontend->LogError("Board " + (string)inst +
                                                    " is now considered dead!");
        }
    }
    keep_alive_received = false;
    protocol->SendCommand(RUBI_COMMAND_KEEPALIVE, {});
}

void BoardCommunicationHandler::FFDataInbound(int ffid,
                                              std::vector<uint8_t> &data)
{
    ASSERT(frontend);

    frontend->FFDataInbound(data, ffid);
};

void BoardCommunicationHandler::ConfirmAddress()
{
    ASSERT(!addressed);
    inst = BoardInstance(shared_from_this());
    // FIXME: behaviour in case of address conflict

    addressed = true;
}

void BoardCommunicationHandler::Launch(sptr<FrontendBoardHandler> _frontend)
{
    protocol->SendCommand(RUBI_COMMAND_OPERATIONAL, {});
    frontend = _frontend;
}

sptr<FrontendBoardHandler> BoardCommunicationHandler::GetFrontendHandler()
{
    return frontend;
}

void BoardCommunicationHandler::Hold()
{
    protocol->SendCommand(RUBI_COMMAND_HOLD, {});
}

void BoardCommunicationHandler::CommandReboot()
{
    dead = true;
    protocol->SendCommand(RUBI_COMMAND_REBOOT, {});
    BoardManager::inst().frontend->LogInfo("Board " + (string)inst +
                                           " ordered was ordered a reboot!");
}

void BoardCommunicationHandler::HandshakeComplete()
{
    string board_name = inst.descriptor->board_name;

    if (BoardManager::inst().descriptor_map.find(board_name) ==
        BoardManager::inst().descriptor_map.end())
    {
        BoardManager::inst().descriptor_map[board_name] = inst.descriptor;
    }
    else
    {
        if (*BoardManager::inst().descriptor_map[board_name] !=
            *inst.descriptor)
        {
            BoardManager::inst().frontend->LogError(
                (std::string) "Descriptor conflict for board + " + board_name +
                "!");
            ASSERT(0);
        }
    }

    BoardManager::inst().frontend->LogInfo("Handshake complete for board " +
                                           board_name + "!");
}