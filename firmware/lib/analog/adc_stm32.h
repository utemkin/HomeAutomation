#pragma once

#include <analog/adc.h>
#include <common/pin_stm32.h>
#include <common/stm32.h>
#include <memory>

namespace Analog
{
  std::unique_ptr<Adc> CreateAdcStm32(const Pin::Def& select2, Adc::Callback&& callback);
}
