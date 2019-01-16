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
  #if defined(STM32F1)
  #elif defined(STM32F4)
    DMALine(MXType* const line, unsigned const channel, uint32_t config, uint32_t fifoControl, uint32_t interruptFlags)
      : m_line(line)
    {
      if (line == DMA1_Stream0)
      {
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_IRQn = DMA1_Stream0_IRQn;
        m_ISR = &DMA1->LISR;
        m_IFCR = &DMA1->LIFCR;
        m_flagsShift = DMA_LISR_FEIF0_Pos;
      }
      else if (line == DMA1_Stream1)
      {
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_IRQn = DMA1_Stream1_IRQn;
        m_ISR = &DMA1->LISR;
        m_IFCR = &DMA1->LIFCR;
        m_flagsShift = DMA_LISR_FEIF1_Pos;
      }
      else if (line == DMA1_Stream2)
      {
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_IRQn = DMA1_Stream2_IRQn;
        m_ISR = &DMA1->LISR;
        m_IFCR = &DMA1->LIFCR;
        m_flagsShift = DMA_LISR_FEIF2_Pos;
      }
      else if (line == DMA1_Stream3)
      {
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_IRQn = DMA1_Stream3_IRQn;
        m_ISR = &DMA1->LISR;
        m_IFCR = &DMA1->LIFCR;
        m_flagsShift = DMA_LISR_FEIF3_Pos;
      }
      else if (line == DMA1_Stream4)
      {
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_IRQn = DMA1_Stream4_IRQn;
        m_ISR = &DMA1->HISR;
        m_IFCR = &DMA1->HIFCR;
        m_flagsShift = DMA_HISR_FEIF4_Pos;
      }
      else if (line == DMA1_Stream5)
      {
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_IRQn = DMA1_Stream5_IRQn;
        m_ISR = &DMA1->HISR;
        m_IFCR = &DMA1->HIFCR;
        m_flagsShift = DMA_HISR_FEIF5_Pos;
      }
      else if (line == DMA1_Stream6)
      {
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_IRQn = DMA1_Stream6_IRQn;
        m_ISR = &DMA1->HISR;
        m_IFCR = &DMA1->HIFCR;
        m_flagsShift = DMA_HISR_FEIF6_Pos;
      }
      else if (line == DMA1_Stream7)
      {
        __HAL_RCC_DMA1_CLK_ENABLE();
        m_IRQn = DMA1_Stream7_IRQn;
        m_ISR = &DMA1->HISR;
        m_IFCR = &DMA1->HIFCR;
        m_flagsShift = DMA_HISR_FEIF7_Pos;
      }
      else if (line == DMA2_Stream0)
      {
        __HAL_RCC_DMA2_CLK_ENABLE();
        m_IRQn = DMA2_Stream0_IRQn;
        m_ISR = &DMA2->LISR;
        m_IFCR = &DMA2->LIFCR;
        m_flagsShift = DMA_LISR_FEIF0_Pos;
      }
      else if (line == DMA2_Stream1)
      {
        __HAL_RCC_DMA2_CLK_ENABLE();
        m_IRQn = DMA2_Stream1_IRQn;
        m_ISR = &DMA2->LISR;
        m_IFCR = &DMA2->LIFCR;
        m_flagsShift = DMA_LISR_FEIF1_Pos;
      }
      else if (line == DMA2_Stream2)
      {
        __HAL_RCC_DMA2_CLK_ENABLE();
        m_IRQn = DMA2_Stream2_IRQn;
        m_ISR = &DMA2->LISR;
        m_IFCR = &DMA2->LIFCR;
        m_flagsShift = DMA_LISR_FEIF2_Pos;
      }
      else if (line == DMA2_Stream3)
      {
        __HAL_RCC_DMA2_CLK_ENABLE();
        m_IRQn = DMA2_Stream3_IRQn;
        m_ISR = &DMA2->LISR;
        m_IFCR = &DMA2->LIFCR;
        m_flagsShift = DMA_LISR_FEIF3_Pos;
      }
      else if (line == DMA2_Stream4)
      {
        __HAL_RCC_DMA2_CLK_ENABLE();
        m_IRQn = DMA2_Stream4_IRQn;
        m_ISR = &DMA2->HISR;
        m_IFCR = &DMA2->HIFCR;
        m_flagsShift = DMA_HISR_FEIF4_Pos;
      }
      else if (line == DMA2_Stream5)
      {
        __HAL_RCC_DMA2_CLK_ENABLE();
        m_IRQn = DMA2_Stream5_IRQn;
        m_ISR = &DMA2->HISR;
        m_IFCR = &DMA2->HIFCR;
        m_flagsShift = DMA_HISR_FEIF5_Pos;
      }
      else if (line == DMA2_Stream6)
      {
        __HAL_RCC_DMA2_CLK_ENABLE();
        m_IRQn = DMA2_Stream6_IRQn;
        m_ISR = &DMA2->HISR;
        m_IFCR = &DMA2->HIFCR;
        m_flagsShift = DMA_HISR_FEIF6_Pos;
      }
      else if (line == DMA2_Stream7)
      {
        __HAL_RCC_DMA2_CLK_ENABLE();
        m_IRQn = DMA2_Stream7_IRQn;
        m_ISR = &DMA2->HISR;
        m_IFCR = &DMA2->HIFCR;
        m_flagsShift = DMA_HISR_FEIF7_Pos;
      }
      else
      {
        //fixme
      }
      m_flagsMask = (DMA_LISR_TCIF0 | DMA_LISR_HTIF0 | DMA_LISR_TEIF0 | DMA_LISR_DMEIF0 | DMA_LISR_FEIF0) << m_flagsShift;
      uint32_t CR = config | (channel << DMA_SxCR_CHSEL_Pos) | DMA_SxCR_EN;
      uint32_t FCR = fifoControl;
      if (interruptFlags & c_flags_TC)
        CR |= DMA_SxCR_TCIE;
      if (interruptFlags & c_flags_HT)
        CR |= DMA_SxCR_HTIE;
      if (interruptFlags & c_flags_TE)
        CR |= DMA_SxCR_TEIE;
      if (interruptFlags & c_flags_DME)
        CR |= DMA_SxCR_DMEIE;
      if (interruptFlags & c_flags_FE)
        FCR |= DMA_SxFCR_FEIE;
      m_CR = CR;
      m_FCR = FCR;
    }
  #else
  #error Unsupported architecture
  #endif
    MXType* line() const
    {
      return m_line;
    }
    IRQn_Type IRQn() const
    {
      return m_IRQn;
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
    void setPAR(uint32_t par)
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      m_line->PAR = par;
  #else
  #error Unsupported architecture
  #endif
    }
    void setMAR(uint32_t mar)
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      m_line->M0AR = mar;
  #else
  #error Unsupported architecture
  #endif
    }
  #if defined(STM32F4)
    void setMAR2(uint32_t mar2)
    {
      m_line->M1AR = mar2;
    }
  #endif
    void start()
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      __DMB();
      m_line->FCR = m_FCR;
      m_line->CR = m_CR;
  #else
  #error Unsupported architecture
  #endif
    }
    void stop()
    {
  #if defined(STM32F1)
  #elif defined(STM32F4)
      m_line->CR = 0;
      while (m_line->CR & DMA_SxCR_EN);  //fixme: check timeout?
      __DMB();
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
    MXType* const m_line;
    IRQn_Type m_IRQn;
    __IO uint32_t* m_ISR;
    __IO uint32_t* m_IFCR;
    uint8_t m_flagsShift;
    uint32_t m_flagsMask;
  #if defined(STM32F1)
  #elif defined(STM32F4)
    uint32_t m_CR;
    uint32_t m_FCR;
  #else
  #error Unsupported architecture
  #endif
  };
}
