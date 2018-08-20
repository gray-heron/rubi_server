#pragma once

#include <linux/can.h>
#include <linux/can/raw.h>
#include <boost/optional/optional.hpp>
#include <inttypes.h>
#include <vector>
#include <sys/ioctl.h>
#include <tuple>

class SocketCan {
    int soc;
    int read_can_port;

    bool is_interface_online(std::string port);

public:
    int can_close();
    int can_send(std::pair<uint16_t, std::vector<uint8_t>> data);
    
    boost::optional<std::tuple<uint16_t, std::vector<uint8_t>, timeval>>
        can_receive(uint32_t timeout_ms);

    SocketCan(std::string port);
};

