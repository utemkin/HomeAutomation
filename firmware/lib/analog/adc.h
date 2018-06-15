#pragma once

#include <lib/common/utils.h>
#include <cstddef>
#include <cstdint>

namespace Analog
{
  class Adc : mstd::noncopyable
  {
  public:
    //must be ready to be called from handler context
    using Callback = mstd::Callback<void>;

  public:
    virtual ~Adc() = default;
    virtual const volatile uint16_t* channel(size_t num) const = 0;
    virtual void start() = 0;
  };
}
