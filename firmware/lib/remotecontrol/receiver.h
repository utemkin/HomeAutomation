#pragma once

#include <lib/common/utils.h>
#include <cstddef>
#include <cstdint>

namespace RC
{
  class Receiver : mstd::noncopyable
  {
  public:
    using DurationUs = int16_t;

  public:
    virtual bool receive(DurationUs& durationUs) = 0;
  };
}
