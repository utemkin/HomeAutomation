#include "hal.h"
#include "rttstreams.h"
#include <lib/segger/SEGGER_RTT.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static size_t writes(void *ip, const uint8_t *bp, size_t n) {

  (void)ip;

  return SEGGER_RTT_Write(0, bp, n);
}

static size_t reads(void *ip, uint8_t *bp, size_t n) {

  (void)ip;

  while (!SEGGER_RTT_HasKey());

  return SEGGER_RTT_Read(0, bp, n);
}

static msg_t put(void *ip, uint8_t b) {

  (void)ip;

  return SEGGER_RTT_Write(0, &b, 1) == 1 ? MSG_OK : MSG_RESET;
}

static msg_t get(void *ip) {

  (void)ip;
  uint8_t b;

  while (!SEGGER_RTT_HasKey());

  return SEGGER_RTT_Read(0, &b, 1) == 1 ? b : MSG_RESET;
}

static const struct RttStreamVMT vmt = {(size_t)0, writes, reads, put, get};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Rtt stream object initialization.
 *
 * @param[out] nsp      pointer to the @p RttStream object to be initialized
 */
void rttObjectInit(RttStream *nsp) {

  nsp->vmt = &vmt;
}
