#pragma once
#include <enc28j60/enc28j60.h>
#include <common/stm32.h>
#include <memory>

namespace Enc28j60
{
  std::unique_ptr<Spi> CreateSpiStm32(SPI_TypeDef* spi, GPIO_TypeDef* csGPIO, uint16_t csPin, bool csInvert);
}
