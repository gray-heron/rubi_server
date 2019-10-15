#include "protocol.h"
#include "exceptions.h"

#include <algorithm>

ProtocolHandler::ProtocolHandler(BoardCommunicationHandler *_board_handler,
                                 uint8_t _board_nodeid,
                                 CanHandler *_can_handler)
    : can_handler(_can_handler), board_handler(_board_handler),
      board_nodeid(_board_nodeid)
{
    rubi_tx_cursor_high = 0;
    rubi_tx_cursor_low = 0;
}

void ProtocolHandler::rubi_inbound(CanRxMsg rx)
{
    if ((rx.StdId >= RUBI_ADDRESS_RANGE1_LOW &&
         rx.StdId <= RUBI_ADDRESS_RANGE1_HIGH) ||
        rx.StdId == RUBI_BROADCAST1)
    {
        uint8_t *potential_data_ptr = &rx.Data[2], data_size;
        ASSERT(rx.DLC >= 1);

        if ((rx.Data[0] & RUBI_MSG_MASK) != RUBI_MSG_BLOCK)
        {
            bool transfer_failed = false;
            ASSERT(rx.DLC >= 2);
            data_size = rx.DLC - 2;

            if (rx.Data[0] & RUBI_FLAG_BLOCK_TRANSFER)
            {
                if (rx.Data[2] != blocks_received)
                {
                    log.Warning("Block transfer has failed.");
                    transfer_failed = true;
                }

                potential_data_ptr = rubi_rx_buffer;
                data_size = rubi_rx_cursor;
                rubi_rx_cursor = 0;
                blocks_received = 0;
            }

            if (!transfer_failed)
                rubi_data_outwrapper(rx.Data[0] & RUBI_MSG_MASK, rx.Data[1],
                                     potential_data_ptr, data_size);
        }
        else
        {
            ASSERT(rx.DLC > 1);
            uint32_t data_len = rx.DLC - 1;

            memcpy(&rubi_rx_buffer[rubi_rx_cursor], &rx.Data[1], data_len);
            rubi_rx_cursor += data_len;
            blocks_received += 1;
        }
    }
}

void ProtocolHandler::InboundWrapper(
    std::pair<uint16_t, std::vector<uint8_t>> msg)
{
    CanRxMsg rx;
    rx.DLC = msg.second.size();
    rx.StdId = msg.first;
    memcpy(rx.Data, msg.second.data(), rx.DLC);

    rubi_inbound(rx);
}

void ProtocolHandler::rubi_data_outwrapper(uint8_t msg_id, uint8_t id,
                                           uint8_t *data, uint8_t datasize)
{
    std::vector<uint8_t> vdata;
    vdata.resize(datasize);
    memcpy(vdata.data(), data, datasize);

    switch (msg_id)
    {
    case RUBI_MSG_FUNCTION:
    case RUBI_MSG_FIELD:
        board_handler->FFDataInbound(id, vdata);
        break;

    case RUBI_MSG_INFO:
        board_handler->DescriptionDataInbound(id, vdata);
        break;

    case RUBI_MSG_EVENT:
        board_handler->EventInbound(id, vdata);
        break;

    case RUBI_MSG_COMMAND:
        board_handler->CommandInbound(id, vdata);
        break;

    default:
        ASSERT(0);
    }
}

uint8_t *ProtocolHandler::rubi_get_tx_chunk(uint8_t size)
{
    static uint8_t buf[8];

    if (rubi_tx_cursor_high + size > RUBI_BUFFER_SIZE)
    {
        memcpy(buf, &rubi_tx_buffer[rubi_tx_cursor_high],
               RUBI_BUFFER_SIZE - rubi_tx_cursor_high);
        memcpy(&buf[RUBI_BUFFER_SIZE - rubi_tx_cursor_high], rubi_tx_buffer,
               size - (RUBI_BUFFER_SIZE - rubi_tx_cursor_high));

        return buf;
    }

    return &rubi_tx_buffer[rubi_tx_cursor_high];
}

// NOT counting the header
int32_t ProtocolHandler::rubi_tx_avaliable_space()
{
    if (rubi_tx_cursor_low == -1)
        return 0;

    if (rubi_tx_cursor_high > rubi_tx_cursor_low)
    {
        return rubi_tx_cursor_high - rubi_tx_cursor_low;
    }

    return RUBI_BUFFER_SIZE - rubi_tx_cursor_low + rubi_tx_cursor_high;
}

void ProtocolHandler::rubi_tx_checkfull()
{
    if (rubi_tx_cursor_high == rubi_tx_cursor_low)
        rubi_tx_cursor_low = -1;
}

// WARNING! this assumes the data will fit
void ProtocolHandler::rubi_tx_enqueue_back(uint8_t *data, int32_t size)
{
    if (!size)
        return;

    if (rubi_tx_cursor_high > rubi_tx_cursor_low ||
        RUBI_BUFFER_SIZE - rubi_tx_cursor_low >= size)
    {
        memcpy(&rubi_tx_buffer[rubi_tx_cursor_low], data, size);
        rubi_tx_cursor_low = (rubi_tx_cursor_low + size) % RUBI_BUFFER_SIZE;
    }
    else
    {
        memcpy(&rubi_tx_buffer[rubi_tx_cursor_low], data,
               RUBI_BUFFER_SIZE - rubi_tx_cursor_low);
        memcpy(rubi_tx_buffer, &data[RUBI_BUFFER_SIZE - rubi_tx_cursor_low],
               size - (RUBI_BUFFER_SIZE - rubi_tx_cursor_low));

        rubi_tx_cursor_low = size - (RUBI_BUFFER_SIZE - rubi_tx_cursor_low);
    }

    rubi_tx_checkfull();
}

// WARNING! this assumes the data will fit
void ProtocolHandler::rubi_tx_enqueue_front(uint8_t *data, int32_t size)
{
    if (!size)
        return;

    if (rubi_tx_cursor_high > rubi_tx_cursor_low || rubi_tx_cursor_high >= size)
    {
        memcpy(&rubi_tx_buffer[rubi_tx_cursor_high - size], data, size);
        rubi_tx_cursor_high -= size;
    }
    else
    {
        memcpy(&rubi_tx_buffer[RUBI_BUFFER_SIZE - (size - rubi_tx_cursor_high)],
               data, size - rubi_tx_cursor_high);
        memcpy(rubi_tx_buffer, &data[size - rubi_tx_cursor_high],
               rubi_tx_cursor_high);

        rubi_tx_cursor_high = RUBI_BUFFER_SIZE - (size - rubi_tx_cursor_high);
    }

    rubi_tx_checkfull();
}

// NOT counting the header
void ProtocolHandler::rubi_wait_for_tx_and_flock(int32_t datasize)
{
    for (;;)
    {
        while (rubi_tx_avaliable_space() < (datasize + sizeof(rubi_dataheader)))
            ;

        rubi_flock();

        if (rubi_tx_avaliable_space() < datasize + sizeof(rubi_dataheader))
        {
            rubi_funlock();
        }
        else
        {
            break;
        }
    }
}

uint32_t ProtocolHandler::rubi_packed_size(uint32_t size)
{
    return size + size / 7 + 2; // data size, block headers, msg_type, msg_size
}

void ProtocolHandler::rubi_continue_tx()
{
    uint8_t data[8];
    rubi_flock();

    for (;;)
    { //! can_mailbox_full()
        if (rubi_tx_current_header.msg_type == 0 &&
            rubi_tx_avaliable_space() == RUBI_BUFFER_SIZE)
        {
            break;
        }

        if (rubi_tx_current_header.msg_type == 0)
        {
            rubi_tx_current_header = *(
                (rubi_dataheader *)rubi_get_tx_chunk(sizeof(rubi_dataheader)));

            block_transfer = rubi_tx_current_header.data_len > 6;
            if (block_transfer)
            {
                rubi_tx_current_header.msg_type |= RUBI_FLAG_BLOCK_TRANSFER;
                blocks_sent = 0;
            }

            if (rubi_tx_cursor_low == -1)
                rubi_tx_cursor_low = rubi_tx_cursor_high;

            rubi_tx_cursor_high =
                (rubi_tx_cursor_high + sizeof(rubi_dataheader)) %
                RUBI_BUFFER_SIZE;
        }

        if (block_transfer && rubi_tx_current_header.data_len != 0)
        {
            uint32_t data_size =
                std::min(7, (int)rubi_tx_current_header.data_len);
            data[0] = RUBI_MSG_BLOCK;
            memcpy((void *)&data[1], rubi_get_tx_chunk(data_size), data_size);

            can_send_array(rubi_tx_current_header.cob, data_size + 1, data);

            if (rubi_tx_cursor_low == -1)
                rubi_tx_cursor_low = rubi_tx_cursor_high;

            rubi_tx_cursor_high =
                (rubi_tx_cursor_high + data_size) % RUBI_BUFFER_SIZE;
            rubi_tx_current_header.data_len -= data_size;

            blocks_sent += 1;
        }
        else
        {
            data[0] = rubi_tx_current_header.msg_type;
            data[1] = rubi_tx_current_header.submsg_type;

            if (block_transfer)
            {
                memcpy(data + 2, &blocks_sent, sizeof(blocks_sent));
                can_send_array(rubi_tx_current_header.cob, 3, data);
                blocks_sent = 0;
            }
            else
            {
                memcpy(data + 2,
                       rubi_get_tx_chunk(rubi_tx_current_header.data_len),
                       rubi_tx_current_header.data_len);
                can_send_array(rubi_tx_current_header.cob,
                               rubi_tx_current_header.data_len + 2, data);
            }

            if (rubi_tx_cursor_low == -1 && rubi_tx_current_header.data_len > 0)
            {
                rubi_tx_cursor_low = rubi_tx_cursor_high;
            }

            rubi_tx_cursor_high =
                (rubi_tx_cursor_high + rubi_tx_current_header.data_len) %
                RUBI_BUFFER_SIZE;

            rubi_tx_current_header.msg_type = 0;
        }
    }

    rubi_funlock();
}

void ProtocolHandler::can_send_array(uint16_t cob, int32_t size,
                                     const uint8_t *data)
{
    std::vector<uint8_t> vdata;
    vdata.resize(size);
    memcpy(vdata.data(), data, size);
    can_handler->socketcan->Send(
        std::pair<uint16_t, std::vector<uint8_t>>(cob, vdata));
}

void ProtocolHandler::SendFFData(uint8_t ffid, uint8_t fftype,
                                 const std::vector<uint8_t> &data)
{
    ASSERT(data.size() < 256);
    uint16_t cob = RUBI_ADDRESS_RANGE1_LOW + board_nodeid;

    rubi_dataheader h = {cob, fftype, ffid, (uint8_t)data.size()};

    rubi_tx_enqueue_back((uint8_t *)&h, sizeof(h));
    rubi_tx_enqueue_back((uint8_t *)data.data(), data.size());

    rubi_continue_tx();
}

void ProtocolHandler::SendCommand(uint8_t command_id,
                                  const std::vector<uint8_t> &data)
{
    ASSERT(data.size() < 7);
    uint16_t cob = RUBI_ADDRESS_RANGE1_LOW + board_nodeid;

    auto data_and_header = std::vector<uint8_t>();
    data_and_header.resize(2 + data.size());
    data_and_header[0] = RUBI_MSG_COMMAND;
    data_and_header[1] = command_id;
    std::copy(data.begin(), data.end(), std::back_inserter(data_and_header));

    can_send_array(RUBI_ADDRESS_RANGE1_LOW + board_nodeid,
                   data_and_header.size(), data_and_header.data());
}
