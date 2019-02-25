#pragma once

#include <boost/optional.hpp>
#include <exception>
#include <inttypes.h>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <tuple>

class CanHandler;
class BoardCommunicationHandler;
class ProtocolHandler;
class FrontendBoardHandler;
class CommunicationFaker;

#include "board.h"
#include "descriptors.h"
#include "frontend.h"
#include "logger.h"
#include "protocol.h"
#include "socketcan.h"

class CanHandler
{
    friend class BoardCommunicationHandler;
    friend class ProtocolHandler;

    std::vector<float> traffic;
    uint64_t traffic_reported = 0;

    std::unique_ptr<SocketCan> socketcan;

    std::vector<boost::optional<std::shared_ptr<BoardCommunicationHandler>>>
        address_pool;

    int max_boards_count;
    uint8_t GetFreeAdress();
    uint8_t NewBoard(uint16_t lottery_id);

    std::chrono::system_clock::time_point last_keepalive;

    Logger log{"CanHandler"};

  public:
    CanHandler(std::string can_name);
    uint64_t GetTrafficSoFar(bool reset = false);

    // std::shared_ptr<BoardCommunicationHandler> GetHandler(int board_node_id);
    void Tick(std::chrono::system_clock::time_point time);
};

class BoardCommunicationHandler
    : public std::enable_shared_from_this<BoardCommunicationHandler>
{
    friend class CanHandler;
    friend class CommunicationFaker;

    int keep_alives_missed;
    int received_descriptors;

    bool dead, lost, operational, addressed, keep_alive_received, wake;
    std::unique_ptr<ProtocolHandler> protocol;
    CanHandler *can_handler;

    sptr<FrontendBoardHandler> frontend;
    BoardInstance inst;

    Logger log{"CommunicationHandler"};

  public:
    void Launch(sptr<FrontendBoardHandler> _frontend);
    void KeepAliveRequest();
    void ConfirmAddress();
    void Hold();
    void HandshakeComplete();

    bool IsDead();
    bool IsLost();
    bool IsWake();

    void CommandReboot();
    void CommandWake();
    void CommandSleep();

    BoardInstance GetBoard();
    sptr<FrontendBoardHandler> GetFrontendHandler();

    void DescriptionDataInbound(int desc_type, std::vector<uint8_t> &data);
    void EventInbound(int error_id, std::vector<uint8_t> &data);
    void CommandInbound(int command_id, std::vector<uint8_t> &data);

    void FFDataInbound(int ffid, std::vector<uint8_t> &data);
    void FFDataOutbound(std::shared_ptr<FFDescriptor> desc,
                        std::vector<uint8_t> &data);

    BoardCommunicationHandler(CanHandler *can_handler, uint8_t board_nodeid);
};