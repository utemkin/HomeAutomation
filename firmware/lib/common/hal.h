#pragma once

#include <lib/common/stm32.h>

namespace HAL
{
  class DMALine
  {
  public:
  #if defined(STM32F1)
      using MXType = DMA_Channel_TypeDef;
  #elif defined(STM32F4)
      using MXType = DMA_Stream_TypeDef;
  #else
  #error Unsupported architecture
  #endif

  public:
    DMALine(MXType* line)
      :m_line(line)
    {
    }
    uint32_t flagsGetAndClear()
    {
      uint32_t const flags = *m_ISR & m_flagsMask;
      *m_IFCR = flags;
      return flags >> m_flagsShift;
    }
    uint16_t NDTR() const
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      return m_line->NDTR;
  #else
  #error Unsupported architecture
  #endif
    }
    void setNDTR(uint16_t ndtr)
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      m_line->NDTR = ndtr;
  #else
  #error Unsupported architecture
  #endif
    }
    void setPAR(void* par)
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      m_line->PAR = uint32_t(par);
  #else
  #error Unsupported architecture
  #endif
    }
    void setMAR(void* mar)
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      m_line->M0AR = uint32_t(mar);
  #else
  #error Unsupported architecture
  #endif
    }
    void start()
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      m_line->FCR = m_initialFCR;
      m_line->CR = m_initialCR | DMA_SxCR_EN;
  #else
  #error Unsupported architecture
  #endif
    }
    void stop()
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      m_line->CR = m_initialCR;
      while (m_line->CR != m_initialCR);  //fixme: check timeout?
  #else
  #error Unsupported architecture
  #endif
    }

  public:
  #if defined(STM32F1)
    constexpr static uint32_t c_TCIF = DMA_ISR_TCIF1;
    constexpr static uint32_t c_HTIF = DMA_ISR_HTIF1;
    constexpr static uint32_t c_TEIF = DMA_ISR_TEIF1;
  #elif defined(STM32F4)
    constexpr static uint32_t c_TCIF = DMA_LISR_TCIF0;
    constexpr static uint32_t c_HTIF = DMA_LISR_HTIF0;
    constexpr static uint32_t c_TEIF = DMA_LISR_TEIF0 | DMA_LISR_DMEIF0 | DMA_LISR_FEIF0;
  #else
  #error Unsupported architecture
  #endif

  protected:
    MXType* const m_line;
    __IO uint32_t* m_ISR;
    __IO uint32_t* m_IFCR;
    uint32_t m_flagsMask;
    uint8_t m_flagsShift;
  #if defined(STM32F1)
  #elif defined(STM32F4)
    uint32_t m_initialCR;
    uint32_t m_initialFCR;
  #else
  #error Unsupported architecture
  #endif
  };
}
