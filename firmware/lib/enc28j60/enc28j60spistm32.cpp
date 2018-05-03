#if 0
#define PERIPH_BB(addr) ((__IO uint32_t*)(((uint32_t)(1 ? (addr) : (uint32_t*)0) - PERIPH_BASE + (PERIPH_BB_BASE >> 5)) << 5))

#define TIMEOUT_TICKS (10)

static int reinit(struct enc28j60spi* spi)
{
  struct enc28j60spi_impl* spi_impl = container_of(spi, struct enc28j60spi_impl, iface);
  deinit(spi_impl);
  spi_impl->hdmarx->CPAR = (uint32_t)&spi_impl->hspi->DR;
  spi_impl->hdmatx->CPAR = (uint32_t)&spi_impl->hspi->DR;
  spi_impl->hspi->CR2 = spi_impl->initial_cr2;
  spi_impl->hspi->CR1 = spi_impl->initial_cr1;
  return 0;
}

static int txrx(struct enc28j60spi* spi, uint8_t* txrx, size_t txrx_len)
{
  struct enc28j60spi_impl* spi_impl = container_of(spi, struct enc28j60spi_impl, iface);
  *spi_impl->cs_bsrr = spi_impl->cs_bsrr_select;

  DMA_Channel_TypeDef* hdmarx = spi_impl->hdmarx;
  hdmarx->CMAR = (uint32_t)txrx;
  hdmarx->CNDTR = txrx_len;
  uint32_t rx_ccr = hdmarx->CCR;
  hdmarx->CCR = rx_ccr | DMA_CCR_EN;
  SPI_TypeDef* hspi = spi_impl->hspi;
  hspi->DR = *txrx;
  DMA_Channel_TypeDef* hdmatx = spi_impl->hdmatx;
  hdmatx->CMAR = (uint32_t)txrx+1;
  hdmatx->CNDTR = txrx_len-1;
  uint32_t tx_ccr = hdmatx->CCR;
  hdmatx->CCR = tx_ccr | DMA_CCR_EN;
  TickType_t initial_tick_count = xTaskGetTickCount();
  while (hdmarx->CNDTR != 0)
  {
    if (IS_ELAPSED(initial_tick_count, TIMEOUT_TICKS))
    {
      deinit(spi_impl);
      return 1;
    }
  }
  hdmarx->CCR = rx_ccr;
  hdmatx->CCR = tx_ccr;
  while ((hspi->SR & SPI_SR_BSY) != 0)
  {
    if (IS_ELAPSED(initial_tick_count, TIMEOUT_TICKS))
    {
      deinit(spi_impl);
      return 2;
    }
  }

  *spi_impl->cs_bsrr = spi_impl->cs_bsrr_deselect;
  return 0;
}
#endif

#include <enc28j60/enc28j60spistm32.h>
#include <common/handlers.h>
#include <limits>

namespace Enc28j60
{
  namespace
  {
    class SpiImpl : public Spi
    {
    public:
      SpiImpl(SPI_TypeDef* const spi, GPIO_TypeDef* const csGPIO, uint16_t const csPin, bool const csInvert)
        : m_spi(spi)
        , m_csBsrr(&csGPIO->BSRR)
        , m_csSelect(csInvert ? csPin << 16 : csPin)
        , m_csDeselect(csInvert ? csPin : csPin << 16)
        , m_delayHclk((210ul * (HAL_RCC_GetHCLKFreq() / 1000ul) + 999999ul) / 1000000ul)
        , m_handlerDmaTx(this)
      {
  #if defined(STM32F1)
        if (m_spi == SPI1)
        {
          __HAL_RCC_SPI1_CLK_ENABLE();
          __HAL_RCC_DMA1_CLK_ENABLE();

          m_dma = DMA1;

          m_dmaTx = DMA1_Channel3;
          m_dmaTxFlags = (DMA_ISR_TEIF1 | DMA_ISR_TCIF1) << (3 - 1) * 4;
          m_handlerDmaTx.install(DMA1_Channel3_IRQn);
          HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 5, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

          m_dmaRx = DMA1_Channel2;
        }
  #ifdef SPI2
        else if (m_spi == SPI2)
        {
          __HAL_RCC_SPI2_CLK_ENABLE();
          __HAL_RCC_DMA1_CLK_ENABLE();

          m_dma = DMA1;

          m_dmaTx = DMA1_Channel5;
          m_dmaTxFlags = (DMA_ISR_TEIF1 | DMA_ISR_TCIF1) << (5 - 1) * 4;
          m_handlerDmaTx.install(DMA1_Channel5_IRQn);
          HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 5, 0);
          HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

          m_dmaRx = DMA1_Channel4;
        }
  #endif
  #ifdef SPI3
        else if (m_spi == SPI3)
        {
          __HAL_RCC_SPI3_CLK_ENABLE();
          __HAL_RCC_DMA2_CLK_ENABLE();

          m_dma = DMA2;

          m_dmaTx = DMA2_Channel2;
          m_dmaTxFlags = (DMA_ISR_TEIF1 | DMA_ISR_TCIF1) << (2 - 1) * 4;
          m_handlerDmaTx.install(DMA2_Channel2_IRQn);
          HAL_NVIC_SetPriority(DMA2_Channel2_IRQn, 5, 0);
          HAL_NVIC_EnableIRQ(DMA2_Channel2_IRQn);

          m_dmaRx = DMA2_Channel1;
        }
  #endif
  #else
  #error Unsupported architecture
  #endif
        else
        {
          //fixme
          return;
        }
        //fixme
        reinit();
      }

      void validateState()
      {
//        if ((m_dmaTx->CCR & DMA_CCR_EN) != 0)
//          Error_Handler();
//
//        if ((m_dmaRx->CCR & DMA_CCR_EN) != 0)
//          Error_Handler();
//
//        if ((m_spi->SR & (SPI_SR_BSY | SPI_SR_OVR | SPI_SR_MODF | SPI_SR_TXE | SPI_SR_RXNE)) != SPI_SR_TXE)
//          Error_Handler();
      }
  
      // should be called either at the end of constructor or after deinit()
      virtual int reinit() override
      {
        //fixme: deinit() ?

        *m_csBsrr = m_csDeselect;

        uint32_t pclk = std::numeric_limits<decltype(pclk)>::max();
  #if defined(STM32F1)
        if (m_spi == SPI1)
        {
          __HAL_RCC_SPI1_FORCE_RESET();
          __HAL_RCC_SPI1_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
        }
  #ifdef SPI2
        else if (m_spi == SPI2)
        {
          __HAL_RCC_SPI2_FORCE_RESET();
          __HAL_RCC_SPI2_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
        }
  #endif
  #ifdef SPI3
        else if (m_spi == SPI3)
        {
          __HAL_RCC_SPI3_FORCE_RESET();
          __HAL_RCC_SPI3_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
        }
  #endif
  #else
  #error Unsupported architecture
  #endif

        uint32_t br;
        for (br = 0; br < 8; pclk >>= 1, ++br)
          if (pclk <= (c_maxSpiRate << 1))
            break;

        if (pclk > (c_maxSpiRate << 1))
          return 1;   //fixme

        m_dmaTx->CPAR = m_dmaRx->CPAR = uint32_t(&m_spi->DR);

        m_spi->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_SPE | (br << SPI_CR1_BR_Pos) | SPI_CR1_MSTR;
        m_spi->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;

        validateState();

        return 0;
      }

      virtual int txRx(uint8_t* const txRx, size_t const txRxLen, bool const delay) override
      {
        *m_csBsrr = m_csSelect;
        __DMB();
        //fixme: according to spec there should be at least Tcss = 50ns delay between falling edge of CS and rising edge of first SCK

        validateState();

        SPI_TypeDef* const spi = m_spi;
        spi->DR = *txRx;
        DMA_Channel_TypeDef* const dmaRx = m_dmaRx;
        dmaRx->CMAR = uint32_t(txRx);
        dmaRx->CNDTR = txRxLen;
        dmaRx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_EN;
        DMA_Channel_TypeDef* const dmaTx = m_dmaTx;
        dmaTx->CMAR = uint32_t(txRx+1);
        dmaTx->CNDTR = txRxLen-1;
        dmaTx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_EN;
        while (dmaTx->CNDTR != 0);
        dmaTx->CCR = 0;
        while (dmaRx->CNDTR != 0);
        dmaRx->CCR = 0;
        while ((spi->SR & SPI_SR_BSY) != 0);

        validateState();

        //fixme: according to spec there should be at least Tcsh = (210ns for MAC|MII regusters or 10ns for others) delay between falling edge of last SCK and rising edge of CS
        if (delay)
          RT::stall(m_delayHclk);

        __DMB();
        *m_csBsrr = m_csDeselect;
        //fixme: according to spec CS should be held high at least Tcsd = 50ns

        return 0;
      }

      virtual int txThenTx(uint8_t const txByte, const uint8_t* const tx, size_t const txLen) override
      {
        *m_csBsrr = m_csSelect;
        __DMB();
        //fixme: according to spec there should be at least Tcss = 50ns delay between falling edge of CS and rising edge of first SCK

        validateState();

        SPI_TypeDef* const spi = m_spi;
        spi->DR = txByte;
        DMA_Channel_TypeDef* const dmaTx = m_dmaTx;
        dmaTx->CMAR = uint32_t(tx);
        dmaTx->CNDTR = txLen;
        if (txLen > c_maxBusyLoop)
        {
          m_dma->IFCR = m_dmaTxFlags;
          dmaTx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE | DMA_CCR_EN;
          m_handlerDmaTx.wait();
        }
        else
        {
          dmaTx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_EN;
          while (dmaTx->CNDTR != 0);
        }
        dmaTx->CCR = 0;
        while ((spi->SR & (SPI_SR_BSY | SPI_SR_TXE)) != SPI_SR_TXE);
        spi->DR;
        spi->SR;

        validateState();

        //fixme: according to spec there should be at least Tcsh = 10ns delay between falling edge of last SCK and rising edge of CS

        __DMB();
        *m_csBsrr = m_csDeselect;
        //fixme: according to spec CS should be held high at least Tcsd = 50ns

        return 0;
      }
  
      virtual int txThenRx(uint8_t const txByte, uint8_t* const rx, size_t const rxLen) override
      {
        *m_csBsrr = m_csSelect;
        __DMB();
        //fixme: according to spec there should be at least Tcss = 50ns delay between falling edge of CS and rising edge of first SCK

        validateState();

        SPI_TypeDef* const spi = m_spi;
        spi->DR = txByte;
        DMA_Channel_TypeDef* const dmaRx = m_dmaRx;
        dmaRx->CMAR = uint32_t(rx);
        dmaRx->CNDTR = rxLen;
        DMA_Channel_TypeDef* const dmaTx = m_dmaTx;
        dmaTx->CMAR = uint32_t(rx);
        dmaTx->CNDTR = rxLen-1;
        while ((spi->SR & (SPI_SR_RXNE)) == 0);
        spi->DR;
        spi->DR = 0;
        if (rxLen > c_maxBusyLoop)
        {
          m_dma->IFCR = m_dmaTxFlags;
          dmaRx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_EN;
          dmaTx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE | DMA_CCR_EN;
          m_handlerDmaTx.wait();
        }
        else
        {
          dmaRx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_EN;
          dmaTx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_DIR | DMA_CCR_EN;
          while (dmaTx->CNDTR != 0);
        }
        dmaTx->CCR = 0;
        while (dmaRx->CNDTR != 0);
        dmaRx->CCR = 0;
        while ((spi->SR & SPI_SR_BSY) != 0);

        validateState();

        //fixme: according to spec there should be at least Tcsh = 10ns delay between falling edge of last SCK and rising edge of CS

        __DMB();
        *m_csBsrr = m_csDeselect;
        //fixme: according to spec CS should be held high at least Tcsd = 50ns

        return 0;
      }

      bool handleDmaTx(IRQn_Type)
      {
        uint32_t const clear = m_dma->ISR & m_dmaTxFlags;
        if (clear)
        {
          m_dma->IFCR = clear;
          m_handlerDmaTx.signal();    //distinguish success and error
          return true;
        }

        return false;
      }      

    protected:
      constexpr static uint32_t c_maxSpiRate = 20000000;
      constexpr static uint32_t c_maxBusyLoop = 50;
      SPI_TypeDef* const m_spi;
      __IO uint32_t* const m_csBsrr;
      uint32_t const m_csSelect;
      uint32_t const m_csDeselect;
      uint32_t const m_delayHclk;
      DMA_TypeDef* m_dma;
      DMA_Channel_TypeDef* m_dmaTx;
      uint32_t m_dmaTxFlags;
      DMA_Channel_TypeDef* m_dmaRx;
      Irq::DelegatedHandler<Irq::SignalingHandler, SpiImpl, &SpiImpl::handleDmaTx> m_handlerDmaTx;

    protected:
      void deinit()
      {
        *m_csBsrr = m_csDeselect;
        m_spi->CR1 = 0;
        m_dmaRx->CCR = 0;
        m_dmaTx->CCR = 0;
      }
    };
  }
}

auto Enc28j60::CreateSpiStm32(SPI_TypeDef* const spi, GPIO_TypeDef* const csGPIO, uint16_t const csPin, bool const csInvert) -> std::unique_ptr<Spi>
{
  return std::make_unique<SpiImpl>(spi, csGPIO, csPin, csInvert);
}
