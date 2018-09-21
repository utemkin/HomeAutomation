#pragma once

#include <lib/common/utils.h>
#include <cstddef>
#include <cstdint>

namespace RC
{
  class Decoder : mstd::noncopyable
  {
  public:
    using DurationUs = uint16_t;
    struct Cycle
    {
      DurationUs oneDurationUs;
      DurationUs zeroDurationUs;
    };
    struct Message
    {
      unsigned repeatCount;
      size_t size;
      const Cycle* data;
    };

  public:
    Decoder() = default;

    // - returns true if there is a message
    // - message.data is only valid until next call to process()
    bool process(bool bit, DurationUs durationUs, Message& message);

  protected:
    bool process(Message& message);

  protected:
    constexpr static DurationUs c_syncThreshold = 3500;
    constexpr static size_t c_maxData = 132;
    constexpr static size_t c_minData = 16;
    constexpr static unsigned c_maxSyncCount = 2;

    bool m_lastBit = true;
    Cycle m_lastCycle = {100, 0};
    enum class Phase {
      WaitFirstSync,
      WaitFirstData,
      ReceiveFirstData,
      WaitNextData,
      ReceiveNextData,
    } m_phase = Phase::WaitFirstSync;
    std::array<Cycle, c_maxData> m_data;
    size_t m_dataSize;
    unsigned m_syncCount;
    size_t m_dataPos;
    unsigned m_repeatCount;
  };
}
