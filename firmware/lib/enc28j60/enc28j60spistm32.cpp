#if 0
#undef USE_HAL_DRIVER
#include "enc28j60spi_f1cmsis.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define container_of(ptr, type, member) ((type *)((char *)(1 ? (ptr) : &((type *)0)->member) - offsetof(type, member)))
#define PERIPH_BB(addr) ((__IO uint32_t*)(((uint32_t)(1 ? (addr) : (uint32_t*)0) - PERIPH_BASE + (PERIPH_BB_BASE >> 5)) << 5))
#define IS_ELAPSED(initial_tick_count, timeout_ticks) ((TickType_t)(xTaskGetTickCount() - (initial_tick_count)) > (TickType_t)(timeout_ticks))

#define TIMEOUT_TICKS (10)

struct enc28j60spi_impl
{
  struct enc28j60spi iface;
  SPI_TypeDef* hspi;
  DMA_Channel_TypeDef* hdmatx;
  DMA_Channel_TypeDef* hdmarx;
  uint16_t initial_cr1;
  uint16_t initial_cr2;
  __IO uint32_t* dma_tx_en;
  __IO uint32_t* dma_rx_en;
  __IO uint32_t* cs_bsrr;
  uint32_t cs_bsrr_select;
  uint32_t cs_bsrr_deselect;
  TaskHandle_t task;
  int error;
  struct enc28j60spi_impl* next;
};

static struct enc28j60spi_impl* first = 0;

static void deinit(struct enc28j60spi_impl* spi_impl)
{
  *spi_impl->cs_bsrr = spi_impl->cs_bsrr_deselect;
  spi_impl->hspi->CR1 = spi_impl->initial_cr1 & ~SPI_CR1_SPE;
  spi_impl->hdmarx->CCR &= ~DMA_CCR_EN;
  spi_impl->hdmatx->CCR &= ~DMA_CCR_EN;
}

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

static int tx_then_tx(struct enc28j60spi* spi, const uint8_t* tx, size_t tx_len, const uint8_t* tx2, size_t tx2_len)
{
  struct enc28j60spi_impl* spi_impl = container_of(spi, struct enc28j60spi_impl, iface);
  *spi_impl->cs_bsrr = spi_impl->cs_bsrr_select;

  SPI_TypeDef* hspi = spi_impl->hspi;
  hspi->DR = *tx;
  DMA_Channel_TypeDef* hdmatx = spi_impl->hdmatx;
  hdmatx->CMAR = (uint32_t)tx+1;
  hdmatx->CNDTR = tx_len-1;
  uint32_t tx_ccr = hdmatx->CCR;
  hdmatx->CCR = tx_ccr | DMA_CCR_EN;
  TickType_t initial_tick_count = xTaskGetTickCount();
  while (hdmatx->CNDTR != 0)
  {
    if (IS_ELAPSED(initial_tick_count, TIMEOUT_TICKS))
    {
      deinit(spi_impl);
      return 3;
    }
  }
  hdmatx->CCR = tx_ccr;
  hdmatx->CMAR = (uint32_t)tx2;
  hdmatx->CNDTR = tx2_len;
  hdmatx->CCR = tx_ccr | DMA_CCR_EN;
  while (hdmatx->CNDTR != 0)
  {
    if (IS_ELAPSED(initial_tick_count, TIMEOUT_TICKS))
    {
      deinit(spi_impl);
      return 4;
    }
  }
  hdmatx->CCR = tx_ccr;
  while ((hspi->SR & (SPI_SR_BSY | SPI_SR_TXE)) != SPI_SR_TXE)
  {
    if (IS_ELAPSED(initial_tick_count, TIMEOUT_TICKS))
    {
      deinit(spi_impl);
      return 5;
    }
  }
  hspi->DR;
  hspi->SR;

  *spi_impl->cs_bsrr = spi_impl->cs_bsrr_deselect;
  return 0;
}

static int tx_then_rx(struct enc28j60spi* spi, const uint8_t* tx, size_t tx_len, uint8_t* rx, size_t rx_len)
{
  struct enc28j60spi_impl* spi_impl = container_of(spi, struct enc28j60spi_impl, iface);
  *spi_impl->cs_bsrr = spi_impl->cs_bsrr_select;

  SPI_TypeDef* hspi = spi_impl->hspi;
  hspi->DR = *tx;
  DMA_Channel_TypeDef* hdmatx = spi_impl->hdmatx;
  hdmatx->CMAR = (uint32_t)tx+1;
  hdmatx->CNDTR = tx_len-1;
  uint32_t tx_ccr = hdmatx->CCR;
  hdmatx->CCR = tx_ccr | DMA_CCR_EN;
  TickType_t initial_tick_count = xTaskGetTickCount();
  while (hdmatx->CNDTR != 0)
  {
    if (IS_ELAPSED(initial_tick_count, TIMEOUT_TICKS))
    {
      deinit(spi_impl);
      return 6;
    }
  }
  hdmatx->CCR = tx_ccr;
  DMA_Channel_TypeDef* hdmarx = spi_impl->hdmarx;
  hdmarx->CMAR = (uint32_t)rx;
  hdmarx->CNDTR = rx_len;
  uint32_t rx_ccr = hdmarx->CCR;
  while ((hspi->SR & (SPI_SR_BSY | SPI_SR_TXE)) != SPI_SR_TXE)
  {
    if (IS_ELAPSED(initial_tick_count, TIMEOUT_TICKS))
    {
      deinit(spi_impl);
      return 7;
    }
  }
  hspi->DR;
  hspi->SR;
  hdmarx->CCR = rx_ccr | DMA_CCR_EN;
  hspi->DR = 0;
  hdmatx->CMAR = (uint32_t)rx;
  hdmatx->CNDTR = rx_len-1;
  hdmatx->CCR = tx_ccr | DMA_CCR_EN;
  while (hdmarx->CNDTR != 0)
  {
    if (IS_ELAPSED(initial_tick_count, TIMEOUT_TICKS))
    {
      deinit(spi_impl);
      return 8;
    }
  }
  hdmarx->CCR = rx_ccr;
  hdmatx->CCR = tx_ccr;
  while ((hspi->SR & SPI_SR_BSY) != 0)
  {
    if (IS_ELAPSED(initial_tick_count, TIMEOUT_TICKS))
    {
      deinit(spi_impl);
      return 9;
    }
  }

  *spi_impl->cs_bsrr = spi_impl->cs_bsrr_deselect;
  return 0;
}

int enc28j60spi_f1cmsis_init(
    struct enc28j60spi** iface,
    SPI_TypeDef* hspi,
    __IO uint32_t* cs_bsrr,
    uint32_t cs_bsrr_select,
    uint32_t cs_bsrr_deselect)
{
  struct enc28j60spi_impl* spi_impl = (struct enc28j60spi_impl*)pvPortMalloc(sizeof(*spi_impl));
  if (!spi_impl)
  {
    return -1;
  }
  memset(spi_impl, 0, sizeof(*spi_impl));
  spi_impl->hspi = hspi;
  if (spi_impl->hspi == SPI1)
  {
    spi_impl->hdmatx = DMA1_Channel3;
    spi_impl->hdmarx = DMA1_Channel2;
  }
#ifdef SPI2
  else if (spi_impl->hspi == SPI2)
  {
    spi_impl->hdmatx = DMA1_Channel5;
    spi_impl->hdmarx = DMA1_Channel4;
  }
#endif
#ifdef SPI3
  else if (spi_impl->hspi == SPI3)
  {
    spi_impl->hdmatx = DMA2_Channel2;
    spi_impl->hdmarx = DMA2_Channel1;
  }
#endif
  else
  {
    vPortFree(spi_impl);
    return -2;
  }
  spi_impl->initial_cr1 = (uint16_t)(spi_impl->hspi->CR1 | SPI_CR1_SPE);
  spi_impl->initial_cr2 = (uint16_t)((spi_impl->hspi->CR2 | SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN) & ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE));
  spi_impl->dma_tx_en = PERIPH_BB(&spi_impl->hdmatx->CCR) + DMA_CCR_EN_Pos;
  spi_impl->dma_rx_en = PERIPH_BB(&spi_impl->hdmarx->CCR) + DMA_CCR_EN_Pos;
  spi_impl->cs_bsrr = cs_bsrr;
  spi_impl->cs_bsrr_select = cs_bsrr_select;
  spi_impl->cs_bsrr_deselect = cs_bsrr_deselect;
  reinit(&spi_impl->iface);
  spi_impl->iface.reinit = &reinit;
  spi_impl->iface.txrx = &txrx;
  spi_impl->iface.tx_then_tx = &tx_then_tx;
  spi_impl->iface.tx_then_rx = &tx_then_rx;
  spi_impl->task = xTaskGetCurrentTaskHandle();
  spi_impl->next = first;
  first = spi_impl;
  *iface = &spi_impl->iface;
  return 0;
}

/*
static struct enc28j60spi_impl* find_spi_impl(SPI_HandleTypeDef *hspi)
{
  struct enc28j60spi_impl* f = first;
  while (f)
  {
    if (f->hspi == hspi)
    {
      break;
    }
    f = f->next;
  }
  return f;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  struct enc28j60spi_impl* spi_impl = find_spi_impl(hspi);
  if (spi_impl)
  {
    spi_impl->error = 1;
    osSignalSet(spi_impl->thread, 1);
  }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  struct enc28j60spi_impl* spi_impl = find_spi_impl(hspi);
  if (spi_impl)
  {
    osSignalSet(spi_impl->thread, 1);
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  struct enc28j60spi_impl* spi_impl = find_spi_impl(hspi);
  if (spi_impl)
  {
    osSignalSet(spi_impl->thread, 1);
  }
}
*/

/*
******************************************************************************************
*********************************************
Benchmarking null...
               ... cycle = 558 ns = 40 CLKs
Benchmarking rxtx 1...
               ... cycle = 824 ns = 60 CLKs
Benchmarking rxtx 2...
               ... cycle = 824 ns = 60 CLKs
Benchmarking rxtx 3...
               ... cycle = 824 ns = 60 CLKs
Benchmarking mem_read 1000...
               ... cycle = 1243 ns = 90 CLKs
Benchmarking mem_read 100...
               ... cycle = 1243 ns = 90 CLKs
Benchmarking mem_read 10...
               ... cycle = 1243 ns = 90 CLKs
Benchmarking mem_write 1000...
               ... cycle = 1201 ns = 87 CLKs
Benchmarking mem_write 100...
               ... cycle = 1201 ns = 87 CLKs
Benchmarking mem_write 10...
               ... cycle = 1201 ns = 87 CLKs
Benchmarking phy_read...
               ... cycle = 8948 ns = 644 CLKs
Benchmarking phy_write...
               ... cycle = 5193 ns = 374 CLKs

******************************************************************************************
  SPI_TypeDef* hspi = spi_impl->hspi;
  while (txrx_len)
  {
    hspi->DR = *txrx;
    --txrx_len;
    while ((hspi->SR & SPI_SR_RXNE) == 0);
    *txrx = hspi->DR;
    ++txrx;
  }
  while ((hspi->SR & SPI_SR_BSY) != 0);
*********************************************
Benchmarking null...
               ... cycle = 572 ns = 41 CLKs
Benchmarking rxtx 1...
               ... cycle = 1633 ns = 118 CLKs
Benchmarking rxtx 2...
               ... cycle = 2512 ns = 181 CLKs
Benchmarking rxtx 3...
               ... cycle = 3392 ns = 244 CLKs
Benchmarking mem_read 1000...
               ... cycle = 446855 ns = 32174 CLKs
Benchmarking mem_read 100...
               ... cycle = 47047 ns = 3388 CLKs
Benchmarking mem_read 10...
               ... cycle = 6978 ns = 503 CLKs
Benchmarking mem_write 1000...
               ... cycle = 446056 ns = 32116 CLKs
Benchmarking mem_write 100...
               ... cycle = 46203 ns = 3327 CLKs
Benchmarking mem_write 10...
               ... cycle = 6200 ns = 447 CLKs
Benchmarking phy_read...
               ... cycle = 31985 ns = 2303 CLKs
Benchmarking phy_write...
               ... cycle = 23112 ns = 1664 CLKs

******************************************************************************************
  DMA_Channel_TypeDef* hdmarx = spi_impl->hdmarx;
  DMA_Channel_TypeDef* hdmatx = spi_impl->hdmatx;
  hdmarx->CMAR = (uint32_t)txrx;
  hdmatx->CMAR = (uint32_t)txrx;
  hdmarx->CNDTR = txrx_len;
  hdmatx->CNDTR = txrx_len;
  uint32_t rx_ccr = hdmarx->CCR;
  uint32_t tx_ccr = hdmatx->CCR;
  hdmarx->CCR = rx_ccr | DMA_CCR_EN;
  hdmatx->CCR = tx_ccr | DMA_CCR_EN;
  SPI_TypeDef* hspi = spi_impl->hspi;
  while (hdmarx->CNDTR != 0);
  hdmarx->CCR = rx_ccr;
  hdmatx->CCR = tx_ccr;
  while ((hspi->SR & SPI_SR_BSY) != 0);
*********************************************
Benchmarking null...
               ... cycle = 572 ns = 41 CLKs
Benchmarking rxtx 1...
               ... cycle = 1786 ns = 129 CLKs
Benchmarking rxtx 2...
               ... cycle = 2131 ns = 154 CLKs
Benchmarking rxtx 3...
               ... cycle = 2593 ns = 187 CLKs
Benchmarking mem_read 1000...
               ... cycle = 446855 ns = 32174 CLKs
Benchmarking mem_read 100...
               ... cycle = 47047 ns = 3388 CLKs
Benchmarking mem_read 10...
               ... cycle = 7004 ns = 504 CLKs
Benchmarking mem_write 1000...
               ... cycle = 446056 ns = 32116 CLKs
Benchmarking mem_write 100...
               ... cycle = 46190 ns = 3326 CLKs
Benchmarking mem_write 10...
               ... cycle = 6185 ns = 445 CLKs
Benchmarking phy_read...
               ... cycle = 29278 ns = 2108 CLKs
Benchmarking phy_write...
               ... cycle = 21984 ns = 1583 CLKs

******************************************************************************************
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
  while (hdmarx->CNDTR != 0);
  hdmarx->CCR = rx_ccr;
  hdmatx->CCR = tx_ccr;
  while ((hspi->SR & SPI_SR_BSY) != 0);
*********************************************
Benchmarking null...
               ... cycle = 572 ns = 41 CLKs
Benchmarking rxtx 1...
               ... cycle = 1689 ns = 122 CLKs
Benchmarking rxtx 2...
               ... cycle = 2145 ns = 155 CLKs
Benchmarking rxtx 3...
               ... cycle = 2510 ns = 181 CLKs
Benchmarking mem_read 1000...
               ... cycle = 446855 ns = 32174 CLKs
Benchmarking mem_read 100...
               ... cycle = 47047 ns = 3388 CLKs
Benchmarking mem_read 10...
               ... cycle = 7062 ns = 509 CLKs
Benchmarking mem_write 1000...
               ... cycle = 446056 ns = 32116 CLKs
Benchmarking mem_write 100...
               ... cycle = 46190 ns = 3326 CLKs
Benchmarking mem_write 10...
               ... cycle = 6185 ns = 445 CLKs
Benchmarking phy_read...
               ... cycle = 29229 ns = 2105 CLKs
Benchmarking phy_write...
               ... cycle = 22082 ns = 1590 CLKs

******************************************************************************************
  DMA_Channel_TypeDef* hdmarx = spi_impl->hdmarx;
  hdmarx->CMAR = (uint32_t)txrx;
  hdmarx->CNDTR = txrx_len;
  *spi_impl->dma_rx_en = 1;
  SPI_TypeDef* hspi = spi_impl->hspi;
  hspi->DR = *txrx;
  DMA_Channel_TypeDef* hdmatx = spi_impl->hdmatx;
  hdmatx->CMAR = (uint32_t)txrx+1;
  hdmatx->CNDTR = txrx_len-1;
  *spi_impl->dma_tx_en = 1;
  while (hdmarx->CNDTR != 0);
  *spi_impl->dma_rx_en = 0;
  *spi_impl->dma_tx_en = 0;
  while ((hspi->SR & SPI_SR_BSY) != 0);
*********************************************
Benchmarking null...
               ... cycle = 572 ns = 41 CLKs
Benchmarking rxtx 1...
               ... cycle = 1716 ns = 124 CLKs
Benchmarking rxtx 2...
               ... cycle = 2245 ns = 162 CLKs
Benchmarking rxtx 3...
               ... cycle = 2623 ns = 189 CLKs
Benchmarking mem_read 1000...
               ... cycle = 446855 ns = 32174 CLKs
Benchmarking mem_read 100...
               ... cycle = 47045 ns = 3387 CLKs
Benchmarking mem_read 10...
               ... cycle = 6928 ns = 499 CLKs
Benchmarking mem_write 1000...
               ... cycle = 446056 ns = 32116 CLKs
Benchmarking mem_write 100...
               ... cycle = 46190 ns = 3326 CLKs
Benchmarking mem_write 10...
               ... cycle = 6185 ns = 445 CLKs
Benchmarking phy_read...
               ... cycle = 29629 ns = 2133 CLKs
Benchmarking phy_write...
               ... cycle = 22220 ns = 1600 CLKs
*/
#endif

#include <enc28j60/enc28j60spistm32.h>
#include <limits>

namespace
{
  class Enc28j60spiStm32 : public Enc28j60spi
  {
  public:
    Enc28j60spiStm32(SPI_TypeDef* spi, GPIO_TypeDef* csGPIO, uint16_t csPin, bool csInvert)
      : m_spi(spi)
    {
#if defined(STM32F1)
      if (m_spi == SPI1)
      {
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_dmatx = DMA1_Channel3;
        m_dmarx = DMA1_Channel2;
      }
#ifdef SPI2
      else if (m_spi == SPI2)
      {
        __HAL_RCC_SPI2_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_dmatx = DMA1_Channel5;
        m_dmarx = DMA1_Channel4;
      }
#endif
#ifdef SPI3
      else if (m_spi == SPI3)
      {
        __HAL_RCC_SPI3_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();
        m_dmatx = DMA2_Channel2;
        m_dmarx = DMA2_Channel1;
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
  
    // should be called either at the end of constructor or after deinit()
    virtual int reinit() override
    {
      //fixme: deinit() ?

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
        if (pclk <= (20000000 << 1))
          break;

      if (pclk > (20000000 << 1))
        return 1;   //fixme

      m_dmatx->CPAR = m_dmarx->CPAR = uint32_t(&m_spi->DR);
      m_dmarx->CCR = DMA_CCR_PL_0 | DMA_CCR_MINC; // | DMA_CCR_EN;
      m_dmatx->CCR = DMA_CCR_PL_0 | DMA_CCR_MINC | DMA_CCR_DIR; // | DMA_CCR_EN;

      m_spi->CR1 = SPI_CR1_SPE | (br << SPI_CR1_BR_Pos) | SPI_CR1_MSTR;
      m_spi->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;

      return 0;
    }
  
    virtual int txrx(uint8_t* txrx, size_t txrx_len) override
    {
      DMA_Channel_TypeDef* const hdmarx = m_dmarx;
      hdmarx->CMAR = uint32_t(txrx);
      hdmarx->CNDTR = txrx_len;
      hdmarx->CCR = DMA_CCR_PL_0 | DMA_CCR_MINC | DMA_CCR_EN;
      SPI_TypeDef* const spi = m_spi;
      spi->DR = *txrx;
      DMA_Channel_TypeDef* const hdmatx = m_dmatx;
      hdmatx->CMAR = uint32_t(txrx+1);
      hdmatx->CNDTR = txrx_len-1;
      hdmatx->CCR = DMA_CCR_PL_0 | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_EN;
      while (hdmarx->CNDTR != 0);
      hdmarx->CCR = 0;
      hdmatx->CCR = 0;
      while ((spi->SR & SPI_SR_BSY) != 0);
/*
      DMA_Channel_TypeDef* const hdmarx = m_dmarx;
      hdmarx->CMAR = uint32_t(txrx);
      hdmarx->CNDTR = txrx_len;
      hdmarx->CCR = DMA_CCR_PL_0 | DMA_CCR_MINC | DMA_CCR_EN;
      SPI_TypeDef* const spi = m_spi;
      spi->DR = *txrx;
      DMA_Channel_TypeDef* const hdmatx = m_dmatx;
      hdmatx->CMAR = uint32_t(txrx+1);
      hdmatx->CNDTR = txrx_len-1;
      hdmatx->CCR = DMA_CCR_PL_0 | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_EN;
      while (hdmarx->CNDTR != 0);
      hdmarx->CCR = 0;
      hdmatx->CCR = 0;
      while ((spi->SR & SPI_SR_BSY) != 0);

Benchmarking rxtx 1...
               ... cycle = 1522 ns = 110 CLKs
Benchmarking rxtx 2...
               ... cycle = 1969 ns = 142 CLKs
Benchmarking rxtx 3...
               ... cycle = 2417 ns = 174 CLKs
*/
/*
      DMA_Channel_TypeDef* const hdmarx = m_dmarx;
      hdmarx->CMAR = uint32_t(txrx);
      hdmarx->CNDTR = txrx_len;
      uint32_t rx_ccr = hdmarx->CCR;
      hdmarx->CCR = rx_ccr | DMA_CCR_EN;
      SPI_TypeDef* const spi = m_spi;
      spi->DR = *txrx;
      DMA_Channel_TypeDef* const hdmatx = m_dmatx;
      hdmatx->CMAR = uint32_t(txrx+1);
      hdmatx->CNDTR = txrx_len-1;
      uint32_t tx_ccr = hdmatx->CCR;
      hdmatx->CCR = tx_ccr | DMA_CCR_EN;
      while (hdmarx->CNDTR != 0);
      hdmarx->CCR = rx_ccr;
      hdmatx->CCR = tx_ccr;
      while ((spi->SR & SPI_SR_BSY) != 0);

Benchmarking rxtx 1...
               ... cycle = 1649 ns = 119 CLKs
Benchmarking rxtx 2...
               ... cycle = 2108 ns = 152 CLKs
Benchmarking rxtx 3...
               ... cycle = 2471 ns = 178 CLKs
*/

      return 0;
    }

    virtual int txThenTx(const uint8_t* tx, size_t tx_len, const uint8_t* tx2, size_t tx2_len) override
    {
      //fixme
      return 1;
    }
  
    virtual int txThenRx(const uint8_t* tx, size_t tx_len, uint8_t* rx, size_t rx_len) override
    {
      //fixme
      return 1;
    }

  protected:
    SPI_TypeDef* const m_spi;
    DMA_Channel_TypeDef* m_dmatx;
    DMA_Channel_TypeDef* m_dmarx;

  protected:
    void deinit()
    {
      //fixme: deselect
      m_spi->CR1 = 0;
      m_dmarx->CCR = 0;
      m_dmatx->CCR = 0;
    }
  };
}

std::unique_ptr<Enc28j60spi> CreateEnc28j60spiStm32(SPI_TypeDef* spi, GPIO_TypeDef* csGPIO, uint16_t csPin, bool csInvert)
{
  return std::make_unique<Enc28j60spiStm32>(spi, csGPIO, csPin, csInvert);
}