#pragma once

#include <lib/analog/adc.h>
#include <lib/common/pin_stm32.h>
#include <lib/common/stm32.h>
#include <memory>

namespace Analog
{
#if defined(STM32F1)
  std::unique_ptr<Adc> CreateAdcStm32(const Pin::Def& select2, Adc::Callback&& callback);
#elif defined(STM32F4)
  std::unique_ptr<Adc> CreateAdcStm32(Adc::Callback&& callback);
#else
#error Unsupported architecture
#endif
}
