

#include "exceptions.h"
#include "socketcan.h"
#include "types.h"

#include <thread>

// http://stackoverflow.com/questions/15723061/how-to-check-if-interface-is-up
bool SocketCan::IsInterfaceAvaliable(std::string port)
{
    struct ifreq ifr;
    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, port.c_str());
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
    {
        perror("SIOCGIFFLAGS");
    }
    close(sock);
    return ifr.ifr_flags & IFF_UP;
}

SocketCan::~SocketCan()
{
    close(soc);
}

size_t SocketCan::GetTotalReceivedDataSize() { return rx_data_n; }

size_t SocketCan::GetTotalTransmittedDataSize() { return tx_data_n; }

bool SocketCan::Send(std::pair<uint16_t, std::vector<uint8_t>> data, bool block)
{
    int retval;
    can_frame frame;

    frame.can_dlc = data.second.size();
    memcpy(frame.data, data.second.data(), frame.can_dlc);
    frame.can_id = data.first;

    do{
        retval = write(soc, &frame, sizeof(struct can_frame));

        if (retval == sizeof(struct can_frame)) {
            tx_data_n += data.second.size();
            return true;
        }

        std::this_thread::yield();
    } while(block);

    return false;
}

boost::optional<std::tuple<uint16_t, std::vector<uint8_t>, timeval>>
SocketCan::Receive(uint32_t timeout_ms)
{
    struct can_frame frame_rd;

    msghdr msg = smsg;
    cmsghdr *cmsg;
    timeval tv;
    std::vector<uint8_t> data;

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(soc, &readSet);
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    if (select((soc + 1), &readSet, NULL, NULL, &timeout) >= 0)
    {
        if (FD_ISSET(soc, &readSet))
        {
            iov.iov_len = sizeof(can_frame);

            auto nbytes_can = recvmsg(soc, &msg, 0);
            for (cmsg = CMSG_FIRSTHDR(&msg);
                 cmsg && (cmsg->cmsg_level == SOL_SOCKET);
                 cmsg = CMSG_NXTHDR(&msg, cmsg))
            {
                if (cmsg->cmsg_type == SO_TIMESTAMP)
                {
                    tv = *(struct timeval *)CMSG_DATA(cmsg);
                }
                else if (cmsg->cmsg_type == SO_RXQ_OVFL)
                {
                    uint32_t dropped_new = *(uint32_t *)CMSG_DATA(cmsg);
                    if (dropped_new > dropped) 
                    {
                        log.Warning("dropped can frames due to receive buffer overflow");
                        dropped = dropped_new;
                    }
                }
            }

            data.resize(frame.can_dlc);
            memcpy(data.data(), frame.data, data.size());

            rx_data_n += data.size();

            return std::tuple<uint16_t, std::vector<uint8_t>, timeval>(
                frame.can_id, data, tv);
        }
    }

    return boost::none;
}

SocketCan::SocketCan(std::string port)
{
    struct ifreq ifr;
    /* open socket */

    soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (soc < 0)
    {
        throw new CanFailureException("soc < 0!");
    }
    addr.can_family = AF_CAN;
    strcpy(ifr.ifr_name, port.c_str());
    if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0)
    {
        throw new CanFailureException(std::string("no such interface ") + port +
                                      "!");
    }

    const int timestamp_on = 1;
    if (setsockopt(soc, SOL_SOCKET, SO_TIMESTAMP, &timestamp_on,
                   sizeof(timestamp_on)) < 0)
    {
        throw new CanFailureException("Can't set SO_TIMESTAMP!");
    }

    const int dropmonitor_on = 1;
    if (setsockopt(soc, SOL_SOCKET, SO_RXQ_OVFL, &dropmonitor_on,
                   sizeof(dropmonitor_on)) < 0)
    {
        throw new CanFailureException("Can't set SO_RXQ_OVFL!");
    }

    addr.can_ifindex = ifr.ifr_ifindex;
    fcntl(soc, F_SETFL, O_NONBLOCK);
    if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        throw new CanFailureException("bind fail!");
    }

    if (!IsInterfaceAvaliable(port))
        throw new CanFailureException("Link is down!");

    smsg.msg_namelen = sizeof(addr);
    smsg.msg_controllen = sizeof(ctrlmsg);
    smsg.msg_flags = 0;
    iov.iov_base = &frame;
    smsg.msg_name = &addr;
    smsg.msg_iov = &iov;
    smsg.msg_iovlen = 1;
    smsg.msg_control = &ctrlmsg;
}