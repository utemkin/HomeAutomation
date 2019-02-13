#pragma once

#include <lib/common/stm32.h>

namespace Hal
{
  using Irq = IRQn_Type;

  // DmaLine() constructs DMA line object and switches it to stopped state
  // setNDTR(), setPAR(), setMAR() set corresponding counter/address
  //   - the value is reloaded every time start() is called
  //   - each of them must be called at least once before start()
  // start() switches line to started state and makes DMA line ready to accept requests
  //   - may only be called from stopped state
  // stop() makes sure line is in stopped state, stops accepting requests and stops any possible DMA line activities
  class DmaLine
  {
  public:
  #if defined(STM32F1)
      using Mx = DMA_Channel_TypeDef;
  #elif defined(STM32F4)
      using Mx = DMA_Stream_TypeDef;
  #else
  #error Unsupported architecture
  #endif

  public:
  #if defined(STM32F1)
    DmaLine(Mx* mx, uint32_t config, uint32_t interruptFlags);
  #elif defined(STM32F4)
    DmaLine(Mx* mx, unsigned channel, uint32_t config, uint32_t fifoControl, uint32_t interruptFlags);
  #else
  #error Unsupported architecture
  #endif
    Mx* mx() const
    {
      return m_mx;
    }
    Irq irq() const
    {
      return m_irq;
    }
    uint32_t flagsGetAndClear()
    {
      uint32_t const flags = *m_ISR & m_interruptFlagsMask;
      *m_IFCR = flags;
      return flags >> m_flagsShift;
    }
    uint32_t flagsGetAndClear(uint32_t const flagsMask)
    {
      uint32_t const flags = *m_ISR & (flagsMask << m_flagsShift);
      *m_IFCR = flags;
      return flags >> m_flagsShift;
    }
    uint16_t NDTR() const
    {
  #if defined(STM32F1)
      return m_mx->CNDTR;
  #elif defined(STM32F4)
      return m_mx->NDTR;
  #else
  #error Unsupported architecture
  #endif
    }
    void setNDTR(uint16_t ndtr)
    {
  #if defined(STM32F1)
      m_mx->CNDTR = ndtr;
  #elif defined(STM32F4)
      m_mx->NDTR = ndtr;
  #else
  #error Unsupported architecture
  #endif
    }
    void setPAR(uint32_t par)
    {
  #if defined(STM32F1)
      m_mx->CPAR = par;
  #elif defined(STM32F4)
      m_mx->PAR = par;
  #else
  #error Unsupported architecture
  #endif
    }
    void setMAR(uint32_t mar)
    {
  #if defined(STM32F1)
      m_mx->CMAR = mar;
  #elif defined(STM32F4)
      m_mx->M0AR = mar;
  #else
  #error Unsupported architecture
  #endif
    }
  #if defined(STM32F4)
    void setMAR2(uint32_t mar2)
    {
      m_mx->M1AR = mar2;
    }
  #endif
    void start()
    {
  #if defined(STM32F1)
      __DMB();
      m_mx->CCR = m_CR;
  #elif defined(STM32F4)
      __DMB();
      m_mx->CR = m_CR;
  #else
  #error Unsupported architecture
  #endif
    }
    void stop()
    {
  #if defined(STM32F1)
      m_mx->CCR = 0;
      __DMB();
  #elif defined(STM32F4)
      m_mx->CR = 0;
      while (m_mx->CR & DMA_SxCR_EN);  //fixme: check timeout?
      __DMB();
  #else
  #error Unsupported architecture
  #endif
    }

  public:
  #if defined(STM32F1)
    constexpr static uint32_t c_config_PRIO_LOW = 0;
    constexpr static uint32_t c_config_PRIO_MED = DMA_CCR_PL_0;
    constexpr static uint32_t c_config_PRIO_HIGH = DMA_CCR_PL_1;
    constexpr static uint32_t c_config_PRIO_VERYHIGH = DMA_CCR_PL_0 | DMA_CCR_PL_1;
    constexpr static uint32_t c_config_M8 = 0;
    constexpr static uint32_t c_config_M16 = DMA_CCR_MSIZE_0;
    constexpr static uint32_t c_config_M32 = DMA_CCR_MSIZE_1;
    constexpr static uint32_t c_config_P8 = 0;
    constexpr static uint32_t c_config_P16 = DMA_CCR_PSIZE_0;
    constexpr static uint32_t c_config_P32 = DMA_CCR_PSIZE_1;
    constexpr static uint32_t c_config_MINC = DMA_CCR_MINC;
    constexpr static uint32_t c_config_PINC = DMA_CCR_PINC;
    constexpr static uint32_t c_config_CIRC = DMA_CCR_CIRC;
    constexpr static uint32_t c_config_P2M = 0;
    constexpr static uint32_t c_config_M2P = DMA_CCR_DIR;
    constexpr static uint32_t c_config_M2M = DMA_CCR_MEM2MEM;

    constexpr static uint32_t c_flags_TC = DMA_ISR_TCIF1;
    constexpr static uint32_t c_flags_HT = DMA_ISR_HTIF1;
    constexpr static uint32_t c_flags_TE = DMA_ISR_TEIF1;
    constexpr static uint32_t c_flags_E = c_flags_TE;
  #elif defined(STM32F4)
    constexpr static uint32_t c_config_MBURST_INCR4 = DMA_SxCR_MBURST_0;
    constexpr static uint32_t c_config_MBURST_INCR8 = DMA_SxCR_MBURST_1;
    constexpr static uint32_t c_config_MBURST_INCR16 = DMA_SxCR_MBURST_0 | DMA_SxCR_MBURST_1;
    constexpr static uint32_t c_config_PBURST_INCR4 = DMA_SxCR_PBURST_0;
    constexpr static uint32_t c_config_PBURST_INCR8 = DMA_SxCR_PBURST_1;
    constexpr static uint32_t c_config_PBURST_INCR16 = DMA_SxCR_PBURST_0 | DMA_SxCR_PBURST_1;
    constexpr static uint32_t c_config_CT = DMA_SxCR_CT;
    constexpr static uint32_t c_config_DBM = DMA_SxCR_DBM;
    constexpr static uint32_t c_config_PRIO_LOW = 0;
    constexpr static uint32_t c_config_PRIO_MED = DMA_SxCR_PL_0;
    constexpr static uint32_t c_config_PRIO_HIGH = DMA_SxCR_PL_1;
    constexpr static uint32_t c_config_PRIO_VERYHIGH = DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
    constexpr static uint32_t c_config_PINCOS = DMA_SxCR_PINCOS;
    constexpr static uint32_t c_config_M8 = 0;
    constexpr static uint32_t c_config_M16 = DMA_SxCR_MSIZE_0;
    constexpr static uint32_t c_config_M32 = DMA_SxCR_MSIZE_1;
    constexpr static uint32_t c_config_P8 = 0;
    constexpr static uint32_t c_config_P16 = DMA_SxCR_PSIZE_0;
    constexpr static uint32_t c_config_P32 = DMA_SxCR_PSIZE_1;
    constexpr static uint32_t c_config_MINC = DMA_SxCR_MINC;
    constexpr static uint32_t c_config_PINC = DMA_SxCR_PINC;
    constexpr static uint32_t c_config_CIRC = DMA_SxCR_CIRC;
    constexpr static uint32_t c_config_P2M = 0;
    constexpr static uint32_t c_config_M2P = DMA_SxCR_DIR_0;
    constexpr static uint32_t c_config_M2M = DMA_SxCR_DIR_1;
    constexpr static uint32_t c_config_PFCTRL = DMA_SxCR_PFCTRL;

    constexpr static uint32_t c_fifoControl_DMDIS = DMA_SxFCR_DMDIS;
    constexpr static uint32_t c_fifoControl_THRESH_1DIV4 = 0;
    constexpr static uint32_t c_fifoControl_THRESH_2DIV4 = DMA_SxFCR_FTH_0;
    constexpr static uint32_t c_fifoControl_THRESH_3DIV4 = DMA_SxFCR_FTH_1;
    constexpr static uint32_t c_fifoControl_THRESH_4DIV4 = DMA_SxFCR_FTH_0 | DMA_SxFCR_FTH_1;

    constexpr static uint32_t c_flags_TC = DMA_LISR_TCIF0;
    constexpr static uint32_t c_flags_HT = DMA_LISR_HTIF0;
    constexpr static uint32_t c_flags_TE = DMA_LISR_TEIF0;
    constexpr static uint32_t c_flags_DME = DMA_LISR_DMEIF0;
    constexpr static uint32_t c_flags_FE = DMA_LISR_FEIF0;
    constexpr static uint32_t c_flags_E = c_flags_TE | c_flags_DME | c_flags_FE;
  #else
  #error Unsupported architecture
  #endif

  protected:
    Mx* const m_mx;
    Irq m_irq;
    __IO uint32_t* m_ISR;
    __IO uint32_t* m_IFCR;
    uint8_t m_flagsShift;
    uint32_t m_interruptFlagsMask;
  #if defined(STM32F1)
    uint32_t m_CR;
  #elif defined(STM32F4)
    uint32_t m_CR;
  #else
  #error Unsupported architecture
  #endif
  };
}
