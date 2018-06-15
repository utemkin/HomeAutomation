#pragma once

#include <lib/analog/adc.h>
#include <lib/common/pin_stm32.h>
#include <lib/common/stm32.h>
#include <memory>

namespace Analog
{
  std::unique_ptr<Adc> CreateAdcStm32(const Pin::Def& select2, Adc::Callback&& callback);
}
