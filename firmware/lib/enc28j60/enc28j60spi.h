#pragma once
#include <common/utils.h>
#include <cstdint>
#include <cstddef>

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
class Enc28j60spi : mstd::noncopyable
{
public:
  virtual int reinit() = 0;
  virtual int txrx(uint8_t* txrx, size_t txrx_len) = 0;
  virtual int txThenTx(const uint8_t* tx, size_t tx_len, const uint8_t* tx2, size_t tx2_len) = 0;
  virtual int txThenRx(const uint8_t* tx, size_t tx_len, uint8_t* rx, size_t rx_len) = 0;
};
