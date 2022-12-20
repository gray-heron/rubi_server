#pragma once

#include <inttypes.h>
#include <memory>

#include "board.h"
#include "communication.h"
#include "logger.h"
#include "protocol_defs.h"
#include "socketcan.h"

#define RUBI_BUFFER_SIZE (0xfff + 8 * sizeof(rubi_dataheader))

class BoardCommunicationHandler;
class CanHandler;

struct rubi_dataheader
{
    uint16_t cob;
    uint8_t msg_type;
    uint8_t submsg_type;
    uint8_t data_len;
};

class ProtocolHandler
{
    typedef struct
    {
        uint32_t StdId;
        uint32_t ExtId;
        uint8_t IDE;
        uint8_t RTR;
        uint8_t DLC;
        uint8_t Data[8];
        uint8_t FMI;
    } CanRxMsg;

    int32_t rubi_rx_cursor = 0;
    uint8_t rubi_tx_buffer[RUBI_BUFFER_SIZE];
    uint8_t rubi_rx_buffer[RUBI_BUFFER_SIZE];
    int32_t rubi_tx_cursor_low;
    int32_t rubi_tx_cursor_high;
    rubi_dataheader rubi_tx_current_header = {0, 0, 0};
    uint16_t board_nodeid;
    uint32_t block_transfer, blocks_sent, blocks_received;

    BoardCommunicationHandler *board_handler;
    CanHandler *can_handler;

    uint8_t *rubi_get_tx_chunk(uint8_t size);
    int32_t rubi_tx_avaliable_space();
    void rubi_tx_checkfull();
    uint32_t rubi_packed_size(uint32_t size);
    void rubi_continue_tx();
    void rubi_wait_for_tx_and_flock(int32_t);
    void rubi_tx_enqueue_back(uint8_t *data, int32_t size);
    void rubi_tx_enqueue_front(uint8_t *data, int32_t size);
    void can_send_array(uint16_t cob, int32_t size, const uint8_t *data);
    void rubi_funlock(){};
    void rubi_flock(){};

    void rubi_inbound(CanRxMsg rx);
    void rubi_data_outwrapper(uint8_t msg_id, uint8_t id, uint8_t *data,
                              uint8_t datasize);

    Logger log{"Protocol"};

  public:
    ProtocolHandler(BoardCommunicationHandler *_board_handler,
                    uint8_t _board_nodeid, CanHandler *_can_handler);

    void InboundWrapper(std::pair<uint16_t, std::vector<uint8_t>> msg);
    void SendFFData(uint8_t ffid, uint8_t fftype,
                    const std::vector<uint8_t> &data);
    void SendCommand(uint8_t command_id, const std::vector<uint8_t> &data);
};
