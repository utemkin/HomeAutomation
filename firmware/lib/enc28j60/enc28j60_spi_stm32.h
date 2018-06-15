#pragma once

#include <lib/enc28j60/enc28j60.h>
#include <lib/common/pin_stm32.h>
#include <lib/common/stm32.h>
#include <memory>

namespace Enc28j60
{
  std::unique_ptr<Spi> CreateSpiStm32(SPI_TypeDef* spi, const Pin::Def& cs);
}
