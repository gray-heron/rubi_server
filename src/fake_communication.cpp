#include <memory>

#include "board.h"
#include "exceptions.h"
#include "fake_communication.h"
#include "frontend.h"
#include "protocol_defs.h"
#include "rubi_autodefs.h"

#include <thread>

using std::get;
using std::string;

bool BoardCommunicationHandler::IsDead() { return false; }
bool BoardCommunicationHandler::IsLost() { return false; }
bool BoardCommunicationHandler::IsWake() { return wake; }

sptr<BoardCommunicationHandler>
CommunicationFaker::FakeBoard(std::string board_name,
                              std::vector<std::pair<fields_desc, int>> fields)
{
    int i = 0;
    default_values.push_back(std::vector<boost::any>());

    auto descriptor = std::make_shared<BoardDescriptor>();
    descriptor->board_name = board_name;
    descriptor->description = "Tic tac toe";
    descriptor->driver = "rubi_generic";
    descriptor->version = "dndj";

    for (const auto &field_entry : fields)
    {
        fields_desc field;
        int access;
        std::tie(field, access) = field_entry;

        auto fdescriptor = std::make_shared<FieldDescriptor>(i);

        fdescriptor->name = get<0>(field);
        fdescriptor->subfields_names = std::vector<string>(get<1>(field));
        fdescriptor->typecode = get<2>(field);
        fdescriptor->access = access;

        descriptor->fieldfunctions.push_back(fdescriptor);

        (default_values.end() - 1)->push_back(get<2>(field));
        ++i;
    }

    auto handler = std::make_shared<BoardCommunicationHandler>(nullptr, 0);
    BoardInstance inst(handler);
    inst.descriptor = descriptor;
    inst.backend_handler = handler;

    BoardManager::inst().descriptor_map[descriptor->board_name] = descriptor;

    handler->Launch(BoardManager::inst().frontend->NewBoard(inst));
    handler->inst = inst;

    return handler;
}

void BoardCommunicationHandler::Launch(sptr<FrontendBoardHandler> _frontend)
{
    frontend = _frontend;
}

BoardInstance BoardCommunicationHandler::GetBoard() { return inst; }

void BoardCommunicationHandler::FFDataInbound(int ffid,
                                              std::vector<uint8_t> &data)
{
    frontend->FFDataInbound(data, ffid);
};

void BoardCommunicationHandler::FFDataOutbound(
    std::shared_ptr<FFDescriptor> desc, std::vector<uint8_t> &data)
{
    string msg = "Message for the board: " + inst.descriptor->board_name;
    msg += " ";

    for (auto byte : data)
    {
        msg += std::to_string(byte) + ":";
    }

    msg = msg.substr(0, msg.size() - 1);

    log.Info(msg);
};

BoardCommunicationHandler::BoardCommunicationHandler(CanHandler *can_handler,
                                                     uint8_t board_nodeid)
{
}

CommunicationFaker::CommunicationFaker() {}

void BoardCommunicationHandler::CommandReboot() { ASSERT(0); }
void BoardCommunicationHandler::CommandSleep() { wake = false; }
void BoardCommunicationHandler::CommandWake() { wake = true; }