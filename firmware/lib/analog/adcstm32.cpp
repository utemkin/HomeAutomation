#include <analog/adcstm32.h>
#include <common/handlers.h>
#include <limits>

namespace Analog
{
  namespace
  {
    class AdcImpl : public Adc
    {
    public:
      AdcImpl(GPIO_TypeDef* const switchGPIO, uint16_t const switchPin, bool const switchInvert)
        : m_switchBsrr(&switchGPIO->BSRR)
        , m_switchSelect1(switchInvert ? switchPin << 16 : switchPin)
        , m_switchSelect2(switchInvert ? switchPin : switchPin << 16)
//        , m_handlerDma1Rx(this)
        , m_handlerDma2Rx(this)
      {
  #if defined(STM32F1)
        __HAL_RCC_ADC3_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();
        __HAL_RCC_ADC3_FORCE_RESET();
        __HAL_RCC_ADC3_RELEASE_RESET();

        m_adc3 = ADC3;
        m_adc3->CR1 = ADC_CR1_SCAN;
        m_adc3->CR2 = ADC_CR2_DMA | ADC_CR2_ADON;
        RT::stall(72);                              //fixme: wait Tstab=1uS

        m_adc3->CR2 = ADC_CR2_DMA | ADC_CR2_RSTCAL | ADC_CR2_ADON;
        while (m_adc3->CR2 & ADC_CR2_RSTCAL);
        m_adc3->CR2 = ADC_CR2_DMA | ADC_CR2_CAL | ADC_CR2_ADON;
        while (m_adc3->CR2 & ADC_CR2_CAL);

        m_adc3->SQR1 = ((c_data2Size - 1) << ADC_SQR1_L_Pos);

        static_assert(c_data2Size == 6);
        m_adc3->SQR3 = (ADC_CHANNEL_13 << ADC_SQR3_SQ6_Pos) | 
                       (ADC_CHANNEL_12 << ADC_SQR3_SQ5_Pos) | 
                       (ADC_CHANNEL_11 << ADC_SQR3_SQ4_Pos) | 
                       (ADC_CHANNEL_10 << ADC_SQR3_SQ3_Pos) | 
                       (ADC_CHANNEL_1 << ADC_SQR3_SQ2_Pos) | 
                       (ADC_CHANNEL_0 << ADC_SQR3_SQ1_Pos);

        m_dma2 = DMA2;
        m_dma2Rx = DMA2_Channel5;
        m_dma2RxFlags = (DMA_ISR_TEIF1 | DMA_ISR_TCIF1) << (5 - 1) * 4;
        m_handlerDma2Rx.install(DMA2_Channel4_5_IRQn);
        HAL_NVIC_SetPriority(DMA2_Channel4_5_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(DMA2_Channel4_5_IRQn);

        m_dma2Rx->CPAR = uint32_t(&m_adc3->DR);
        m_dma2Rx->CMAR = uint32_t(&m_data[c_data1Size * 2]);
  #else
  #error Unsupported architecture
  #endif
      }

//      bool handleDma1Rx(IRQn_Type)
//      {
//        uint32_t const clear = m_dma1->ISR & m_dma1RxFlags;
//        if (clear)
//        {
//          m_dma1->IFCR = clear;
//          m_handlerDma1Rx.signal();    //distinguish success and error
//          return true;
//        }
//
//        return false;
//      }      

      virtual void startConversion() override
      {
  #if defined(STM32F1)
        m_dma2Rx->CNDTR = c_data2Size;
        m_dma2Rx->CCR = DMA_CCR_PL_0 | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC | DMA_CCR_EN;

        m_adc3->CR2 = ADC_CR2_DMA | ADC_CR2_ADON;
        OS::Thread::delay(1000);                    //fixme
        m_dma2Rx->CCR = 0;
  #endif
      }
      
      bool handleDma2Rx(IRQn_Type)
      {
        uint32_t const clear = m_dma2->ISR & m_dma2RxFlags;
        if (clear)
        {
          m_dma2->IFCR = clear;
          m_handlerDma2Rx.signal();    //distinguish success and error
          return true;
        }

        return false;
      }      

    protected:
      constexpr static size_t c_data1Size = 7;
      constexpr static size_t c_data2Size = 6;
      __IO uint32_t* const m_switchBsrr;
      uint32_t const m_switchSelect1;
      uint32_t const m_switchSelect2;
//      ADC_TypeDef* m_adc1;
//      ADC_TypeDef* m_adc2;
      ADC_TypeDef* m_adc3;
//      DMA_TypeDef* m_dma1;
//      DMA_Channel_TypeDef* m_dma1Rx;
//      uint32_t m_dma1RxFlags;
//      Irq::DelegatedHandler<Irq::SignalingHandler, AdcImpl, &AdcImpl::handleDma1Rx> m_handlerDma1Rx;
      DMA_TypeDef* m_dma2;
      DMA_Channel_TypeDef* m_dma2Rx;
      uint32_t m_dma2RxFlags;
      Irq::DelegatedHandler<Irq::SignalingHandler, AdcImpl, &AdcImpl::handleDma2Rx> m_handlerDma2Rx;
      uint16_t m_data[c_data1Size * 2 + c_data2Size];
    };
  }
}

auto Analog::CreateAdcStm32(GPIO_TypeDef* const switchGPIO, uint16_t const switchPin, bool const switchInvert) -> std::unique_ptr<Adc>
{
  return std::make_unique<AdcImpl>(switchGPIO, switchPin, switchInvert);
}
