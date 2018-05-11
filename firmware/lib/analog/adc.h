#pragma once

#include <common/utils.h>
#include <cstddef>
#include <cstdint>

namespace Analog
{
  class Adc : mstd::noncopyable
  {
  public:
    virtual ~Adc() = default;
    virtual void convert() = 0;
  };
}
