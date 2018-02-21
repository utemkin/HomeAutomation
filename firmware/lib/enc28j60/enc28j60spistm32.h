#pragma once
#include <enc28j60/enc28j60spi.h>
#include <common/stm32.h>
#include <memory>

std::unique_ptr<Enc28j60spi> CreateEnc28j60spiStm32(SPI_TypeDef* spi, GPIO_TypeDef* csGPIO, uint16_t csPin, bool csInvert);
