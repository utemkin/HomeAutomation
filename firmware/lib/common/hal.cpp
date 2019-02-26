#include <lib/common/hal.h>

namespace Hal
{
#if defined(STM32F1)
  void DmaLine::Setup::update(const Setup& other)
  {
    resource.controller.update(other.resource.controller);
    resource.line.update(other.resource.line);
    config.update(other.config);
    interruptFlags.update(other.interruptFlags);
  }

  Status DmaLine::create(std::unique_ptr<DmaLine>& dmaLine, Setup const& setup)
  {
    struct EnableMaker : public DmaLine {};
    auto&& ptr = std::make_unique<EnableMaker>();

    if (setup.resource.controller.isDefault() || setup.resource.line.isDefault())
      return Status::RequiredParamaterMissing;

    if (setup.config.isDefault())
      return Status::RequiredParamaterMissing;

    if (false)
      ;
    #if defined(DMA1_Channel1)
    else if (setup.resource.controller == 1 && setup.resource.line == 1)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Channel1;
      ptr->m_irq = DMA1_Channel1_IRQn;
      ptr->m_ISR = &DMA1->ISR;
      ptr->m_IFCR = &DMA1->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF1_Pos;
    }
    #endif
    #if defined(DMA1_Channel2)
    else if (setup.resource.controller == 1 && setup.resource.line == 2)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Channel2;
      ptr->m_irq = DMA1_Channel2_IRQn;
      ptr->m_ISR = &DMA1->ISR;
      ptr->m_IFCR = &DMA1->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF2_Pos;
    }
    #endif
    #if defined(DMA1_Channel3)
    else if (setup.resource.controller == 1 && setup.resource.line == 3)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Channel3;
      ptr->m_irq = DMA1_Channel3_IRQn;
      ptr->m_ISR = &DMA1->ISR;
      ptr->m_IFCR = &DMA1->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF3_Pos;
    }
    #endif
    #if defined(DMA1_Channel4)
    else if (setup.resource.controller == 1 && setup.resource.line == 4)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Channel4;
      ptr->m_irq = DMA1_Channel4_IRQn;
      ptr->m_ISR = &DMA1->ISR;
      ptr->m_IFCR = &DMA1->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF4_Pos;
    }
    #endif
    #if defined(DMA1_Channel5)
    else if (setup.resource.controller == 1 && setup.resource.line == 5)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Channel5;
      ptr->m_irq = DMA1_Channel5_IRQn;
      ptr->m_ISR = &DMA1->ISR;
      ptr->m_IFCR = &DMA1->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF5_Pos;
    }
    #endif
    #if defined(DMA1_Channel6)
    else if (setup.resource.controller == 1 && setup.resource.line == 6)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Channel6;
      ptr->m_irq = DMA1_Channel6_IRQn;
      ptr->m_ISR = &DMA1->ISR;
      ptr->m_IFCR = &DMA1->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF6_Pos;
    }
    #endif
    #if defined(DMA1_Channel7)
    else if (setup.resource.controller == 1 && setup.resource.line == 7)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Channel7;
      ptr->m_irq = DMA1_Channel7_IRQn;
      ptr->m_ISR = &DMA1->ISR;
      ptr->m_IFCR = &DMA1->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF7_Pos;
    }
    #endif
    #if defined(DMA2_Channel1)
    else if (setup.resource.controller == 2 && setup.resource.line == 1)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Channel1;
      ptr->m_irq = DMA2_Channel1_IRQn;
      ptr->m_ISR = &DMA2->ISR;
      ptr->m_IFCR = &DMA2->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF1_Pos;
    }
    #endif
    #if defined(DMA2_Channel2)
    else if (setup.resource.controller == 2 && setup.resource.line == 2)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Channel2;
      ptr->m_irq = DMA2_Channel2_IRQn;
      ptr->m_ISR = &DMA2->ISR;
      ptr->m_IFCR = &DMA2->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF2_Pos;
    }
    #endif
    #if defined(DMA2_Channel3)
    else if (setup.resource.controller == 2 && setup.resource.line == 3)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Channel3;
      ptr->m_irq = DMA2_Channel3_IRQn;
      ptr->m_ISR = &DMA2->ISR;
      ptr->m_IFCR = &DMA2->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF3_Pos;
    }
    #endif
    #if defined(DMA2_Channel4)
    else if (setup.resource.controller == 2 && setup.resource.line == 4)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Channel4;
      ptr->m_irq = DMA2_Channel4_5_IRQn;
      ptr->m_ISR = &DMA2->ISR;
      ptr->m_IFCR = &DMA2->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF4_Pos;
    }
    #endif
    #if defined(DMA2_Channel5)
    else if (setup.resource.controller == 2 && setup.resource.line == 5)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Channel5;
      ptr->m_irq = DMA2_Channel4_5_IRQn;
      ptr->m_ISR = &DMA2->ISR;
      ptr->m_IFCR = &DMA2->IFCR;
      ptr->m_flagsShift = DMA_ISR_GIF5_Pos;
    }
    #endif
    else
      return Status::BadParameter;

    ptr->m_interruptFlagsMask = setup.interruptFlags << ptr->m_flagsShift;
    uint32_t CR = setup.config | DMA_CCR_EN;
    if (setup.interruptFlags & c_flags_TC)
      CR |= DMA_CCR_TCIE;
    if (setup.interruptFlags & c_flags_HT)
      CR |= DMA_CCR_HTIE;
    if (setup.interruptFlags & c_flags_TE)
      CR |= DMA_CCR_TEIE;
    ptr->m_CR = CR;

    dmaLine = std::move(ptr);
    return Status::Success;
  }
#elif defined(STM32F4)
  void DmaLine::Setup::update(const Setup& other)
  {
    resource.controller.update(other.resource.controller);
    resource.line.update(other.resource.line);
    channel.update(other.channel);
    config.update(other.config);
    fifoControl.update(other.fifoControl);
    interruptFlags.update(other.interruptFlags);
  }

  Status DmaLine::create(std::unique_ptr<DmaLine>& dmaLine, Setup const& setup)
  {
    struct EnableMaker : public DmaLine {};
    auto&& ptr = std::make_unique<EnableMaker>();

    if (setup.resource.controller.isDefault() || setup.resource.line.isDefault())
      return Status::RequiredParamaterMissing;

    if (setup.channel.isDefault())
      return Status::RequiredParamaterMissing;

    if (setup.config.isDefault())
      return Status::RequiredParamaterMissing;

    if (false)
      ;
    #if defined(DMA1_Stream0)
    else if (setup.resource.controller == 1 && setup.resource.line == 0)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Stream0;
      ptr->m_irq = DMA1_Stream0_IRQn;
      ptr->m_ISR = &DMA1->LISR;
      ptr->m_IFCR = &DMA1->LIFCR;
      ptr->m_flagsShift = DMA_LISR_FEIF0_Pos;
    }
    #endif
    #if defined(DMA1_Stream1)
    else if (setup.resource.controller == 1 && setup.resource.line == 1)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Stream1;
      ptr->m_irq = DMA1_Stream1_IRQn;
      ptr->m_ISR = &DMA1->LISR;
      ptr->m_IFCR = &DMA1->LIFCR;
      ptr->m_flagsShift = DMA_LISR_FEIF1_Pos;
    }
    #endif
    #if defined(DMA1_Stream2)
    else if (setup.resource.controller == 1 && setup.resource.line == 2)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Stream2;
      ptr->m_irq = DMA1_Stream2_IRQn;
      ptr->m_ISR = &DMA1->LISR;
      ptr->m_IFCR = &DMA1->LIFCR;
      ptr->m_flagsShift = DMA_LISR_FEIF2_Pos;
    }
    #endif
    #if defined(DMA1_Stream3)
    else if (setup.resource.controller == 1 && setup.resource.line == 3)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Stream3;
      ptr->m_irq = DMA1_Stream3_IRQn;
      ptr->m_ISR = &DMA1->LISR;
      ptr->m_IFCR = &DMA1->LIFCR;
      ptr->m_flagsShift = DMA_LISR_FEIF3_Pos;
    }
    #endif
    #if defined(DMA1_Stream4)
    else if (setup.resource.controller == 1 && setup.resource.line == 4)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Stream4;
      ptr->m_irq = DMA1_Stream4_IRQn;
      ptr->m_ISR = &DMA1->HISR;
      ptr->m_IFCR = &DMA1->HIFCR;
      ptr->m_flagsShift = DMA_HISR_FEIF4_Pos;
    }
    #endif
    #if defined(DMA1_Stream5)
    else if (setup.resource.controller == 1 && setup.resource.line == 5)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Stream5;
      ptr->m_irq = DMA1_Stream5_IRQn;
      ptr->m_ISR = &DMA1->HISR;
      ptr->m_IFCR = &DMA1->HIFCR;
      ptr->m_flagsShift = DMA_HISR_FEIF5_Pos;
    }
    #endif
    #if defined(DMA1_Stream6)
    else if (setup.resource.controller == 1 && setup.resource.line == 6)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Stream6;
      ptr->m_irq = DMA1_Stream6_IRQn;
      ptr->m_ISR = &DMA1->HISR;
      ptr->m_IFCR = &DMA1->HIFCR;
      ptr->m_flagsShift = DMA_HISR_FEIF6_Pos;
    }
    #endif
    #if defined(DMA1_Stream7)
    else if (setup.resource.controller == 1 && setup.resource.line == 7)
    {
      __HAL_RCC_DMA1_CLK_ENABLE();
      ptr->m_mx = DMA1_Stream7;
      ptr->m_irq = DMA1_Stream7_IRQn;
      ptr->m_ISR = &DMA1->HISR;
      ptr->m_IFCR = &DMA1->HIFCR;
      ptr->m_flagsShift = DMA_HISR_FEIF7_Pos;
    }
    #endif
    #if defined(DMA2_Stream0)
    else if (setup.resource.controller == 2 && setup.resource.line == 0)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Stream0;
      ptr->m_irq = DMA2_Stream0_IRQn;
      ptr->m_ISR = &DMA2->LISR;
      ptr->m_IFCR = &DMA2->LIFCR;
      ptr->m_flagsShift = DMA_LISR_FEIF0_Pos;
    }
    #endif
    #if defined(DMA2_Stream1)
    else if (setup.resource.controller == 2 && setup.resource.line == 1)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Stream1;
      ptr->m_irq = DMA2_Stream1_IRQn;
      ptr->m_ISR = &DMA2->LISR;
      ptr->m_IFCR = &DMA2->LIFCR;
      ptr->m_flagsShift = DMA_LISR_FEIF1_Pos;
    }
    #endif
    #if defined(DMA2_Stream2)
    else if (setup.resource.controller == 2 && setup.resource.line == 2)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Stream2;
      ptr->m_irq = DMA2_Stream2_IRQn;
      ptr->m_ISR = &DMA2->LISR;
      ptr->m_IFCR = &DMA2->LIFCR;
      ptr->m_flagsShift = DMA_LISR_FEIF2_Pos;
    }
    #endif
    #if defined(DMA2_Stream3)
    else if (setup.resource.controller == 2 && setup.resource.line == 3)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Stream3;
      ptr->m_irq = DMA2_Stream3_IRQn;
      ptr->m_ISR = &DMA2->LISR;
      ptr->m_IFCR = &DMA2->LIFCR;
      ptr->m_flagsShift = DMA_LISR_FEIF3_Pos;
    }
    #endif
    #if defined(DMA2_Stream4)
    else if (setup.resource.controller == 2 && setup.resource.line == 4)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Stream4;
      ptr->m_irq = DMA2_Stream4_IRQn;
      ptr->m_ISR = &DMA2->HISR;
      ptr->m_IFCR = &DMA2->HIFCR;
      ptr->m_flagsShift = DMA_HISR_FEIF4_Pos;
    }
    #endif
    #if defined(DMA2_Stream5)
    else if (setup.resource.controller == 2 && setup.resource.line == 5)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Stream5;
      ptr->m_irq = DMA2_Stream5_IRQn;
      ptr->m_ISR = &DMA2->HISR;
      ptr->m_IFCR = &DMA2->HIFCR;
      ptr->m_flagsShift = DMA_HISR_FEIF5_Pos;
    }
    #endif
    #if defined(DMA2_Stream6)
    else if (setup.resource.controller == 2 && setup.resource.line == 6)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Stream6;
      ptr->m_irq = DMA2_Stream6_IRQn;
      ptr->m_ISR = &DMA2->HISR;
      ptr->m_IFCR = &DMA2->HIFCR;
      ptr->m_flagsShift = DMA_HISR_FEIF6_Pos;
    }
    #endif
    #if defined(DMA2_Stream7)
    else if (setup.resource.controller == 2 && setup.resource.line == 7)
    {
      __HAL_RCC_DMA2_CLK_ENABLE();
      ptr->m_mx = DMA2_Stream7;
      ptr->m_irq = DMA2_Stream7_IRQn;
      ptr->m_ISR = &DMA2->HISR;
      ptr->m_IFCR = &DMA2->HIFCR;
      ptr->m_flagsShift = DMA_HISR_FEIF7_Pos;
    }
    #endif
    else
      return Status::BadParameter;

    ptr->m_interruptFlagsMask = setup.interruptFlags << ptr->m_flagsShift;
    ptr->m_allFlagsMask = c_flags_all << ptr->m_flagsShift;
    uint32_t CR = setup.config | mstd::bits_at<DMA_SxCR_CHSEL_Pos>(setup.channel) | DMA_SxCR_EN;
    uint32_t FCR = setup.fifoControl;
    if (setup.interruptFlags & c_flags_TC)
      CR |= DMA_SxCR_TCIE;
    if (setup.interruptFlags & c_flags_HT)
      CR |= DMA_SxCR_HTIE;
    if (setup.interruptFlags & c_flags_TE)
      CR |= DMA_SxCR_TEIE;
    if (setup.interruptFlags & c_flags_DME)
      CR |= DMA_SxCR_DMEIE;
    if (setup.interruptFlags & c_flags_FE)
      FCR |= DMA_SxFCR_FEIE;
    ptr->m_CR = CR;
    ptr->m_mx->FCR = FCR;

    dmaLine = std::move(ptr);
    return Status::Success;
  }
#else
#error Unsupported architecture
#endif
}
