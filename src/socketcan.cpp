#include <cstring>
#include <fcntl.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "commons.h"
#include "exceptions.h"
#include "socketcan.h"

// TODO: Clean this up

#define CAN_FRAME_OVERHEAD 6

char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
msghdr smsg;
struct iovec iov;
struct can_frame frame;

// http://stackoverflow.com/questions/15723061/how-to-check-if-interface-is-up
bool SocketCan::is_interface_online(std::string port)
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
    return !!(ifr.ifr_flags & IFF_UP);
}

int SocketCan::can_close()
{
    close(soc);
    return 0;
}

int SocketCan::GetReceivedDataSize() { return rx_data_n; }

int SocketCan::GetSentDataSize() { return tx_data_n; }

int SocketCan::can_send(std::pair<uint16_t, std::vector<uint8_t>> data)
{
    int retval;
    can_frame frame;

    frame.can_dlc = data.second.size();
    memcpy(frame.data, data.second.data(), frame.can_dlc);
    frame.can_id = data.first;

    retval = write(soc, &frame, sizeof(struct can_frame));
    if (retval != sizeof(struct can_frame))
    {
        throw new CanFailureException("Transmission error!");
    }
    else
    {
        tx_data_n += data.second.size() + CAN_FRAME_OVERHEAD;
        return (0);
    }
}

boost::optional<std::tuple<uint16_t, std::vector<uint8_t>, timeval>>
SocketCan::can_receive(uint32_t timeout_ms)
{
    struct can_frame frame_rd;
    int recvbytes = 0;
    read_can_port = 1;
    /*
            struct timeval timeout = { timeout_ms / 1000,
                (timeout_ms % 1000) * 1000 };

            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(soc, &readSet);
            if (select((soc + 1), &readSet, NULL, NULL, &timeout) >= 0)
            {
                if (FD_ISSET(soc, &readSet))
                {
                    recvbytes = read(soc, &frame_rd, sizeof(struct can_frame));
                    struct timeval tv;
                    ioctl(soc, SIOCGSTAMP, &tv);

                    if (recvbytes)
                    {
                        std::vector<uint8_t> data;
                        data.resize(frame_rd.can_dlc);
                        memcpy(data.data(), frame_rd.data, frame_rd.can_dlc);
                        return std::tuple<uint16_t, std::vector<uint8_t>,
       timeval>
                            (frame_rd.can_id, data, tv);
                    }
                }
            }

    */
    msghdr msg = smsg;
    cmsghdr *cmsg;
    timeval tv;

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(soc, &readSet);
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    if (select((soc + 1), &readSet, NULL, NULL, &timeout) >= 0)
    {
        if (FD_ISSET(soc, &readSet))
        {
            iov.iov_len = sizeof(frame);

            auto nbytes_can = recvmsg(soc, &msg, 0);
            for (cmsg = CMSG_FIRSTHDR(&msg);
                 cmsg && (cmsg->cmsg_level == SOL_SOCKET);
                 cmsg = CMSG_NXTHDR(&msg, cmsg))
            {
                if (cmsg->cmsg_type == SO_TIMESTAMP)
                {
                    tv = *(struct timeval *)CMSG_DATA(cmsg);
                    std::vector<uint8_t> data;
                    data.resize(frame.can_dlc);
                    memcpy(data.data(), frame.data, data.size());
                    rx_data_n += data.size() + CAN_FRAME_OVERHEAD;

                    return std::tuple<uint16_t, std::vector<uint8_t>, timeval>(
                        frame.can_id, data, tv);
                }
            }
        }
    }

    return boost::none;
}

SocketCan::SocketCan(std::string port)
{
    struct ifreq ifr;
    struct sockaddr_can addr;
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
    addr.can_ifindex = ifr.ifr_ifindex;
    fcntl(soc, F_SETFL, O_NONBLOCK);
    if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        throw new CanFailureException("bind fail!");
    }

    if (!is_interface_online(port))
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
