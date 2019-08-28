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

BoardInstance BoardCommunicationHandler::GetBoard() { return inst; }

bool BoardCommunicationHandler::IsDead() { return dead; }
bool BoardCommunicationHandler::IsLost() { return lost; }
bool BoardCommunicationHandler::IsWake() { return wake; }

void BoardCommunicationHandler::FFDataOutbound(
    std::shared_ptr<FFDescriptor> desc, std::vector<uint8_t> &data)
{
    if (IsWake())
        protocol->SendFFData(desc->ffid, desc->GetFFType(), data);
}

BoardCommunicationHandler::BoardCommunicationHandler(CanHandler *can_handler,
                                                     uint8_t board_nodeid)
    : can_handler(can_handler), dead(false), addressed(false),
      operational(false), lost(false), wake(false), keep_alives_missed(0),
      keep_alive_received(true), received_descriptors(0)
{
    protocol = std::unique_ptr<ProtocolHandler>(
        new ProtocolHandler(this, board_nodeid, can_handler));
}

void BoardCommunicationHandler::DescriptionDataInbound(
    const int desc_type, std::vector<uint8_t> &data)
{
    std::string value = DataToString(data);
    if (!inst.descriptor)
    {
        ASSERT(desc_type == RUBI_INFO_BOARD_NAME);

        inst.descriptor = std::make_shared<BoardDescriptor>();
    }

    if (desc_type == RUBI_INFO_BOARD_ID)
    {
        inst.id.emplace(value);
    }
    else
    {
        inst.descriptor->ApplyInfo(desc_type, value);
    }
}

void BoardCommunicationHandler::EventInbound(int error_type,
                                             std::vector<uint8_t> &data)
{
    string msg;
    string error_msg;
    uint32_t error_code;

    switch (error_type)
    {
    case RUBI_EVENT_FATAL_ERROR:
        ASSERT(data.size() > 5);
        for (unsigned int i = 5; i < data.size(); i++)
            error_msg += (char)data[i];

        error_code = *((uint32_t *)(data.data() + 1));

        switch (data[0])
        {
        case RUBI_ERROR_ASSERT:
            msg = "An assertion has fired on board " + (std::string)inst +
                  " in file " + error_msg + " at line " +
                  std::to_string(error_code);
            break;
        case RUBI_ERROR_USER:
            msg = "A fatal user error has occured on board " + (string)inst +
                  ": " + error_msg;
        }

        log.Error(msg);
        break;

    case RUBI_EVENT_INFO:
        ASSERT(data.size() > 1);

        msg = "Info from board " + (std::string)inst + ": ";
        msg += std::string((char *)data.data() + 1);

        log.Info(msg);
        break;

    case RUBI_EVENT_WARNING:
        ASSERT(data.size() > 1);

        msg = "Info from board " + (std::string)inst + ": ";
        msg += std::string((char *)data.data() + 1);

        log.Warning(msg);
        break;

    case RUBI_EVENT_ERROR:
        ASSERT(data.size() > 1);

        msg = "Info from board " + (std::string)inst + ": ";
        msg += std::string((char *)data.data() + 1);

        log.Error(msg);
        break;

    default:
        ASSERT(false);
    }
}

void BoardCommunicationHandler::CommandInbound(int command_id,
                                               std::vector<uint8_t> &data)
{
    ASSERT(command_id == RUBI_COMMAND_KEEPALIVE);
    // TODO keep-alive id

    ASSERT(data.size() == 1);
    wake = data[0];

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

        log.Warning("Didn't receive keep-alive from " + (string)inst + "!");

        if (keep_alives_missed == 5)
        {
            dead = true;
            log.Error("Board " + (string)inst + " is now considered dead!");
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
    ASSERT(!addressed, "Address assignment has failed!");
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
    log.Info("Board " + (string)inst + " was ordered a reboot!");
}

void BoardCommunicationHandler::CommandSleep()
{
    protocol->SendCommand(RUBI_COMMAND_SOFTSLEEP, {});
    log.Info("Board " + (string)inst + " ordered to go sleeping!");
}

void BoardCommunicationHandler::CommandWake()
{
    protocol->SendCommand(RUBI_COMMAND_WAKE, {});
    log.Info("Board " + (string)inst + " was ordered to wake up!");
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
            log.Error((std::string) "Descriptor conflict for board + " +
                      board_name + "!");
            ASSERT(0);
        }
    }

    log.Info("Handshake complete for board " + board_name + "!");
}
