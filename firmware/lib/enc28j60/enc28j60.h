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

#pragma once
#include <enc28j60/enc28j60spi.h>
#include <common/utils.h>
#include <memory>

class Enc28j60Pbuf : mstd::noncopyable
{
public:
  virtual size_t size() const = 0;
  virtual bool next(uint8_t** data, size_t* size) = 0;
  virtual bool next(const uint8_t** data, size_t* size) const = 0;
};

class Enc28j60Services : mstd::noncopyable
{
public:
  virtual std::unique_ptr<Enc28j60Pbuf> allocatePbuf(size_t size) = 0;
  virtual void input(std::unique_ptr<Enc28j60Pbuf>&& packet) = 0;
  virtual void setLinkState(bool linked) = 0;
};

class Enc28j60 : mstd::noncopyable
{
public:
  //virtual void output(std::unique_ptr<Enc28j60Pbuf>&& packet) = 0;
  virtual void test() = 0;
};

std::unique_ptr<Enc28j60> CreateEnc28j60(const std::shared_ptr<Enc28j60spi>& spi);
