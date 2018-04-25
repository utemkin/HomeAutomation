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
#include <enc28j60/enc28j60.h>
#include <common/utils.h>
#include <memory>

namespace Enc28j60
{
  struct MacAddress
  {
    int8_t addr[6];
  };

  class Pbuf : mstd::noncopyable
  {
  public:
    virtual ~Pbuf() = default;
    virtual size_t size() const = 0;
    virtual bool next(uint8_t*& data, size_t& size) = 0;
    virtual bool next(const uint8_t*& data, size_t& size) const = 0;
  };

  class Env : mstd::noncopyable
  {
  public:
    virtual ~Env() = default;
    virtual std::unique_ptr<Pbuf> allocatePbuf(size_t size) = 0;
    virtual void input(std::unique_ptr<Pbuf>&& packet) = 0;
    virtual void setLinkState(bool linked) = 0;
  };

  // - CS must be activated 50ns before transfer start and stay active 210ns after transfer end, then deactivated.
  // - Buffers can't be NULL or zero-sized.
  // - All calls are synchronous and expected to block.
  //   Implementation must decide whether to busy-loop or yield basing on the following hints:
  //     - txrx_len is in range [1, 3]
  //     - tx_len is 1
  //     - tx2_len and rx_len is in range [1, ~1520]
  // - Return value is 0 if no error and non-zero otherwise.
  //   If error occurs, the transfer is aborted at unknown stage, so reinit() must be called before any other call
  // - reinit() must re-initialize SPI hardware and CS pin state, must be able to recover from all function's failures
  // - fixme: ??? Implementation may expect initialization and all subsequent calls to be done from the same thread
  class Spi : mstd::noncopyable
  {
  public:
    virtual ~Spi() = default;
    virtual int reinit() = 0;
    virtual int txRx(uint8_t* txrx, size_t txrxLen) = 0;
    virtual int txThenTx(uint8_t txByte, const uint8_t* tx, size_t txLen) = 0;
    virtual int txThenRx(uint8_t txByte, uint8_t* rx, size_t rxLen) = 0;
  };

  class Device : mstd::noncopyable
  {
  public:
    virtual ~Device() = default;
    virtual bool output(std::unique_ptr<Pbuf>&& packet) = 0;
    virtual void periodic() = 0;
    virtual void test() = 0;
  };

  std::unique_ptr<Device> CreateDevice(std::unique_ptr<Env>&& env, std::unique_ptr<Spi>&& spi, const MacAddress& mac);
}
