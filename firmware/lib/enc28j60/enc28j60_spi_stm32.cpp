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

#include <lib/enc28j60/enc28j60_spi_stm32.h>
#include <lib/common/handlers.h>
#include <limits>

namespace Enc28j60
{
  namespace
  {
    class SpiImpl : public Spi
    {
    public:
      SpiImpl(SPI_TypeDef* const spi, const Pin::Def& cs)
        : m_spi(spi)
        , m_cs(cs)
        , m_delayHclk((210ul * (HAL_RCC_GetHCLKFreq() / 1000ul) + 999999ul) / 1000000ul)
        , m_handlerDmaTx(Irq::Handler::Callback::make<SpiImpl, &SpiImpl::handleDmaTx>(*this))
      {
#if defined(STM32F1)
        Hal::DmaLine::Setup dmaTxSetup = {
          .config = Hal::DmaLine::c_config_PRIO_LOW | Hal::DmaLine::c_config_M8 | Hal::DmaLine::c_config_P8 | Hal::DmaLine::c_config_MINC | Hal::DmaLine::c_config_M2P,
//          .interruptFlags = Hal::DmaLine::c_flags_TC | Hal::DmaLine::c_flags_E,
        };

        Hal::DmaLine::Setup dmaRxSetup = {
          .config = Hal::DmaLine::c_config_PRIO_LOW | Hal::DmaLine::c_config_M8 | Hal::DmaLine::c_config_P8 | Hal::DmaLine::c_config_MINC | Hal::DmaLine::c_config_P2M,
        };

        if (false)
          ;
#  ifdef SPI1
        else if (m_spi == SPI1)
        {
          dmaTxSetup.update( {
            .resource = {
              .controller = 1,
              .line = 3,
            },
          } );

          dmaRxSetup.update( {
            .resource = {
              .controller = 1,
              .line = 2,
            },
          } );

          __HAL_RCC_SPI1_CLK_ENABLE();
        }
#  endif
#  ifdef SPI2
        else if (m_spi == SPI2)
        {
          dmaTxSetup.update( {
            .resource = {
              .controller = 1,
              .line = 5,
            },
          } );

          dmaRxSetup.update( {
            .resource = {
              .controller = 1,
              .line = 4,
            },
          } );

          __HAL_RCC_SPI2_CLK_ENABLE();
        }
#  endif
#  ifdef SPI3
        else if (m_spi == SPI3)
        {
          dmaTxSetup.update( {
            .resource = {
              .controller = 2,
              .line = 2,
            },
          } );

          dmaRxSetup.update( {
            .resource = {
              .controller = 2,
              .line = 1,
            },
          } );

          __HAL_RCC_SPI3_CLK_ENABLE();
        }
#  endif
        else
        {
          Rt::fatal();  //fixme
        }
#elif defined(STM32F4)
        Hal::DmaLine::Setup dmaTxSetup = {
            .config = Hal::DmaLine::c_config_PRIO_LOW | Hal::DmaLine::c_config_M8 | Hal::DmaLine::c_config_P8 | Hal::DmaLine::c_config_MINC | Hal::DmaLine::c_config_M2P,
            .fifoControl = Hal::DmaLine::c_fifoControl_DMDIS | Hal::DmaLine::c_fifoControl_THRESH_2DIV4,
            .interruptFlags = Hal::DmaLine::c_flags_TC | Hal::DmaLine::c_flags_E,
        };

        Hal::DmaLine::Setup dmaRxSetup = {
            .config = Hal::DmaLine::c_config_PRIO_LOW | Hal::DmaLine::c_config_M8 | Hal::DmaLine::c_config_P8 | Hal::DmaLine::c_config_MINC | Hal::DmaLine::c_config_P2M,
            .fifoControl = Hal::DmaLine::c_fifoControl_DMDIS | Hal::DmaLine::c_fifoControl_THRESH_2DIV4,
        };

        if (false)
          ;
#  ifdef SPI1
        else if (m_spi == SPI1)
        {
          dmaTxSetup.update( {
            .resource = {
              .controller = 2,  //fixme
              .line = 3,        //fixme
            },
            .channel = 3,
          } );

          dmaRxSetup.update( {
            .resource = {
              .controller = 2,  //fixme
              .line = 0,        //fixme
            },
            .channel = 3,
          } );

          __HAL_RCC_SPI1_CLK_ENABLE();
        }
#  endif
#  ifdef SPI2
        else if (m_spi == SPI2)
        {
          dmaTxSetup.update( {
            .resource = {
              .controller = 1,  //fixme
              .line = 4,        //fixme
            },
            .channel = 0,
          } );

          dmaRxSetup.update( {
            .resource = {
              .controller = 1,  //fixme
              .line = 3,        //fixme
            },
            .channel = 0,
          } );

          __HAL_RCC_SPI2_CLK_ENABLE();
        }
#  endif
#  ifdef SPI3
        else if (m_spi == SPI3)
        {
          dmaTxSetup.update( {
            .resource = {
              .controller = 1,  //fixme
              .line = 5,        //fixme
            },
            .channel = 0,
          } );

          dmaRxSetup.update( {
            .resource = {
              .controller = 1,  //fixme
              .line = 0,        //fixme
            },
            .channel = 0,
          } );

          __HAL_RCC_SPI3_CLK_ENABLE();
        }
#  endif
#  ifdef SPI4
        else if (m_spi == SPI4)
        {
          dmaTxSetup.update( {
            .resource = {
              .controller = 2,  //fixme
              .line = 1,        //fixme
            },
            .channel = 4,
          } );

          dmaRxSetup.update( {
            .resource = {
              .controller = 2,  //fixme
              .line = 0,        //fixme
            },
            .channel = 4,
          } );

          __HAL_RCC_SPI4_CLK_ENABLE();
        }
#  endif
#  ifdef SPI5
        else if (m_spi == SPI5)
        {
          dmaTxSetup.update( {
            .resource = {
              .controller = 2,  //fixme
              .line = 4,        //fixme
            },
            .channel = 2,
          } );

          dmaRxSetup.update( {
            .resource = {
              .controller = 2,  //fixme
              .line = 3,        //fixme
            },
            .channel = 2,
          } );

          __HAL_RCC_SPI5_CLK_ENABLE();
        }
#  endif
        else
        {
          Rt::fatal();  //fixme
        }
#else
#  error Unsupported architecture
#endif
        if (Hal::DmaLine::create(m_dmaTx, dmaTxSetup) != Hal::Status::Success)
          Rt::fatal();  //fixme

        if (Hal::DmaLine::create(m_dmaRx, dmaRxSetup) != Hal::Status::Success)
          Rt::fatal();  //fixme

        m_handlerDmaTx.install(m_dmaTx->irq());
        HAL_NVIC_SetPriority(m_dmaTx->irq(), 5, 0);
        HAL_NVIC_EnableIRQ(m_dmaTx->irq());

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

        m_cs.toPassive();

        uint32_t pclk = std::numeric_limits<decltype(pclk)>::max();
#if defined(STM32F1)
        if (false)
          ;
#  ifdef SPI1
        if (m_spi == SPI1)
        {
          __HAL_RCC_SPI1_FORCE_RESET();
          __HAL_RCC_SPI1_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
        }
#  endif
#  ifdef SPI2
        else if (m_spi == SPI2)
        {
          __HAL_RCC_SPI2_FORCE_RESET();
          __HAL_RCC_SPI2_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
        }
#  endif
#  ifdef SPI3
        else if (m_spi == SPI3)
        {
          __HAL_RCC_SPI3_FORCE_RESET();
          __HAL_RCC_SPI3_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
        }
#  endif
#elif defined(STM32F4)
        if (false)
          ;
#  ifdef SPI1
        if (m_spi == SPI1)
        {
          __HAL_RCC_SPI1_FORCE_RESET();
          __HAL_RCC_SPI1_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
        }
#  endif
#  ifdef SPI2
        else if (m_spi == SPI2)
        {
          __HAL_RCC_SPI2_FORCE_RESET();
          __HAL_RCC_SPI2_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
        }
#  endif
#  ifdef SPI3
        else if (m_spi == SPI3)
        {
          __HAL_RCC_SPI3_FORCE_RESET();
          __HAL_RCC_SPI3_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
        }
#  endif
#  ifdef SPI4
        else if (m_spi == SPI4)
        {
          __HAL_RCC_SPI4_FORCE_RESET();
          __HAL_RCC_SPI4_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
        }
#  endif
#  ifdef SPI5
        else if (m_spi == SPI5)
        {
          __HAL_RCC_SPI5_FORCE_RESET();
          __HAL_RCC_SPI5_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
        }
#  endif
#else
#  error Unsupported architecture
#endif

        uint32_t br;
        for (br = 0; br < 8; pclk >>= 1, ++br)
          if (pclk <= (c_maxSpiRate << 1))
            break;

        if (pclk > (c_maxSpiRate << 1))
          return 1;   //fixme

        m_dmaTx->setPAR(uint32_t(&m_spi->DR));
        m_dmaRx->setPAR(uint32_t(&m_spi->DR));

        m_spi->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_SPE | (br << SPI_CR1_BR_Pos) | SPI_CR1_MSTR;
        m_spi->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;

        validateState();

        return 0;
      }

      virtual int txRx(uint8_t* const txRx, size_t const txRxLen, bool const delay) override
      {
        m_cs.toActive();
        __DMB();
        //fixme: according to spec there should be at least Tcss = 50ns delay between falling edge of CS and rising edge of first SCK

        validateState();

        SPI_TypeDef* const spi = m_spi;
        spi->DR = *txRx;
        auto const dmaRx = m_dmaRx.get();
        dmaRx->setMAR(uint32_t(txRx));
        dmaRx->setNDTR(txRxLen);
        dmaRx->start();
        auto const dmaTx = m_dmaTx.get();
        dmaTx->setMAR(uint32_t(txRx+1));
        dmaTx->setNDTR(txRxLen-1);
        dmaTx->start();
        while (dmaTx->NDTR() != 0);
        dmaTx->stop();
        while (dmaRx->NDTR() != 0);
        dmaRx->stop();
        while ((spi->SR & SPI_SR_BSY) != 0);

        validateState();

        //fixme: according to spec there should be at least Tcsh = (210ns for MAC|MII regusters or 10ns for others) delay between falling edge of last SCK and rising edge of CS
        if (delay)
          Rt::stall(m_delayHclk);

        __DMB();
        m_cs.toPassive();
        //fixme: according to spec CS should be held high at least Tcsd = 50ns

        return 0;
      }

      virtual int txThenTx(uint8_t const txByte, const uint8_t* const tx, size_t const txLen) override
      {
        m_cs.toActive();
        __DMB();
        //fixme: according to spec there should be at least Tcss = 50ns delay between falling edge of CS and rising edge of first SCK

        validateState();

        SPI_TypeDef* const spi = m_spi;
        spi->DR = txByte;
        auto const dmaTx = m_dmaTx.get();
        dmaTx->setMAR(uint32_t(tx));
        dmaTx->setNDTR(txLen);
//        if (txLen > c_maxBusyLoop)
//        {
//          m_dma->IFCR = m_dmaTxFlags;
//          dmaTx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE | DMA_CCR_EN;
//          m_handlerDmaTx.wait();
//        }
//        else
//        {
          dmaTx->start();
          while (dmaTx->NDTR() != 0);
//        }
        dmaTx->stop();
        while ((spi->SR & (SPI_SR_BSY | SPI_SR_TXE)) != SPI_SR_TXE);
        spi->DR;
        spi->SR;

        validateState();

        //fixme: according to spec there should be at least Tcsh = 10ns delay between falling edge of last SCK and rising edge of CS

        __DMB();
        m_cs.toPassive();
        //fixme: according to spec CS should be held high at least Tcsd = 50ns

        return 0;
      }
  
      virtual int txThenRx(uint8_t const txByte, uint8_t* const rx, size_t const rxLen) override
      {
        m_cs.toActive();
        __DMB();
        //fixme: according to spec there should be at least Tcss = 50ns delay between falling edge of CS and rising edge of first SCK

        validateState();

        SPI_TypeDef* const spi = m_spi;
        spi->DR = txByte;
        auto const dmaRx = m_dmaRx.get();
        dmaRx->setMAR(uint32_t(rx));
        dmaRx->setNDTR(rxLen);
        auto const dmaTx = m_dmaTx.get();
        dmaTx->setMAR(uint32_t(rx));
        dmaTx->setNDTR(rxLen-1);
        while ((spi->SR & (SPI_SR_RXNE)) == 0);
        spi->DR;
        spi->DR = 0;
//        if (rxLen > c_maxBusyLoop)
//        {
//          m_dma->IFCR = m_dmaTxFlags;
//          dmaRx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_EN;
//          dmaTx->CCR = DMA_CCR_PL_0 | DMA_CCR_PSIZE_1 | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_TEIE | DMA_CCR_EN;
//          m_handlerDmaTx.wait();
//        }
//        else
//        {
          dmaRx->start();
          dmaTx->start();
          while (dmaTx->NDTR() != 0);
//        }
        dmaTx->stop();
        while (dmaRx->NDTR() != 0);
        dmaRx->stop();
        while ((spi->SR & SPI_SR_BSY) != 0);

        validateState();

        //fixme: according to spec there should be at least Tcsh = 10ns delay between falling edge of last SCK and rising edge of CS

        __DMB();
        m_cs.toPassive();
        //fixme: according to spec CS should be held high at least Tcsd = 50ns

        return 0;
      }

    protected:
      bool handleDmaTx(Hal::Irq)
      {
        auto const flags = m_dmaTx->flagsGetAndClear();
        if (flags)
        {
          m_handlerDmaTx.signal();    //fixme: distinguish success and error
          return true;
        }

        return false;
      }      

    protected:
      constexpr static uint32_t c_maxSpiRate = 20000000;
      constexpr static uint32_t c_maxBusyLoop = 50;
      SPI_TypeDef* const m_spi;
      Pin::Out const m_cs;
      uint32_t const m_delayHclk;
      Irq::SemaphoreHandler m_handlerDmaTx;
      std::unique_ptr<Hal::DmaLine> m_dmaTx;
      std::unique_ptr<Hal::DmaLine> m_dmaRx;

    protected:
      void deinit()
      {
        m_cs.toPassive();
        m_spi->CR1 = 0;
        m_dmaRx->stop();
        m_dmaTx->stop();
      }
    };
  }
}

auto Enc28j60::CreateSpiStm32(SPI_TypeDef* const spi, const Pin::Def& cs) -> std::unique_ptr<Spi>
{
  return std::make_unique<SpiImpl>(spi, cs);
}
