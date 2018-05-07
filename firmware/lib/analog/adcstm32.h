#pragma once
#include <analog/adc.h>
#include <common/stm32.h>
#include <memory>

namespace Analog
{
  std::unique_ptr<Adc> CreateAdcStm32(GPIO_TypeDef* switchGPIO, uint16_t switchPin, bool switchInvert);
}
