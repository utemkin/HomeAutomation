#ifndef RTTSTREAMS_H
#define RTTSTREAMS_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   @p RttStream specific data.
 */
#define _rtt_stream_data                                                   \
  _base_sequential_stream_data

/**
 * @brief   @p RttStream virtual methods table.
 */
struct RttStreamVMT {
  _base_sequential_stream_methods
};

/**
 * @extends BaseSequentialStream
 *
 * @brief   Rtt stream object.
 */
typedef struct {
  /** @brief Virtual Methods Table.*/
  const struct RttStreamVMT *vmt;
  _rtt_stream_data
} RttStream;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void rttObjectInit(RttStream *nsp);
#ifdef __cplusplus
}
#endif

#endif /* RTTSTREAMS_H */
