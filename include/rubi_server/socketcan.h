#pragma once

#include <inttypes.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <tuple>
#include <vector>
#include <cstring>
#include <boost/optional/optional.hpp>

#include "logger.h"

class SocketCan
{
    int soc;
    int read_can_port;
    size_t rx_data_n = 0;
    size_t tx_data_n = 0;
    uint32_t dropped = 0;

    struct can_frame frame;
    struct sockaddr_can addr;
    char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
    msghdr smsg;
    struct iovec iov;

    Logger log{"SocketCan"};

  public:
    static bool IsInterfaceAvaliable(std::string port);

    size_t GetTotalReceivedDataSize();
    size_t GetTotalTransmittedDataSize();

    bool Send(std::pair<uint16_t, std::vector<uint8_t>> data, bool block=true);
    boost::optional<std::tuple<uint16_t, std::vector<uint8_t>, timeval>>
    Receive(uint32_t timeout_ms);

    SocketCan(std::string port);
    ~SocketCan();
};
