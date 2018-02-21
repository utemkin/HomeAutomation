#pragma once
#include <enc28j60/enc28j60spi.h>
#include <cstdint>
#include <cstddef>

/*
#include <stdint.h>
#include <stddef.h>

#define ENC28J60_MEM_SIZE (8192)

struct enc28j60
{
};

extern int enc28j60_init(
    struct enc28j60** enc,
    struct enc28j60spi* spi,
    uint16_t rx_buf_size,
    uint16_t max_packet_size);

extern void enc28j60_poll(
    struct enc28j60* enc);

extern int enc28j60_packet_read_try_start(
    struct enc28j60* enc,
    uint16_t* len);
extern int enc28j60_packet_read_part(
    struct enc28j60* enc);
extern int enc28j60_packet_read_finish(
    struct enc28j60* enc);
*/

class Enc28j60
{
public:
  Enc28j60();

protected:
  Enc28j60spi& m_spi;
};
