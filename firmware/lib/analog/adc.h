#pragma once

#include <common/utils.h>
#include <cstddef>
#include <cstdint>

namespace Analog
{
  class Adc : mstd::noncopyable
  {
  public:
    //must be ready to be called from handler context
    using Callback = mstd::Callback<void, uint16_t*, size_t>;

  public:
    virtual ~Adc() = default;
    virtual void start() = 0;
  };
}
