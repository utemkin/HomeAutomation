#include <lib/common/hal.h>

namespace HAL
{
#if defined(STM32F1)
#elif defined(STM32F4)
  DMALine::DMALine(MXType* const line, unsigned const channel, uint32_t config, uint32_t fifoControl, uint32_t interruptFlags)
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
}
