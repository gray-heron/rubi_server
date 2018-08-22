#pragma once

#include <boost/optional/optional.hpp>
#include <inttypes.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <tuple>
#include <vector>

class SocketCan
{
    // TODO: correct naming

    int soc;
    int read_can_port;

    bool is_interface_online(std::string port);
    int rx_data_n = 0;
    int tx_data_n = 0;

  public:
    int can_close();
    int can_send(std::pair<uint16_t, std::vector<uint8_t>> data);

    int GetReceivedDataSize();
    int GetSentDataSize();

    boost::optional<std::tuple<uint16_t, std::vector<uint8_t>, timeval>>
    can_receive(uint32_t timeout_ms);

    SocketCan(std::string port);
};
