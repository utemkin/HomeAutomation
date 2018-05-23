#include <analog/adcstm32.h>
#include <common/handlers.h>
#include <limits>

extern "C" uint32_t tm[10];
uint32_t tm[10];

namespace Analog
{
  namespace
  {
    class AdcImpl : public Adc
    {
    public:
      AdcImpl(GPIO_TypeDef* const switchGPIO, uint16_t const switchPin, bool const switchInvert)
        : m_switchBsrr(switchGPIO ? &switchGPIO->BSRR : nullptr)
        , m_switchSelect1(switchInvert ? switchPin << 16 : switchPin)
        , m_switchSelect2(switchInvert ? switchPin : switchPin << 16)
        , m_handlerDma1Rx(this)
        , m_handlerDma2Rx(this)
      {
#if defined(STM32F1)
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_ADC1_FORCE_RESET();
        __HAL_RCC_ADC1_RELEASE_RESET();
        m_adc1 = ADC1;
        m_adc1->CR1 = (6 << ADC_CR1_DUALMOD_Pos) | ADC_CR1_SCAN;
        m_adc1->CR2 = ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_ADON;
        RT::stall(72);                              //fixme: wait Tstab=1uS
        m_adc1->CR2 = ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_RSTCAL | ADC_CR2_ADON;
        while (m_adc1->CR2 & ADC_CR2_RSTCAL);
        m_adc1->CR2 = ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_CAL | ADC_CR2_ADON;
        while (m_adc1->CR2 & ADC_CR2_CAL);
        m_adc1->SQR1 = ((c_data1Size - 1) << ADC_SQR1_L_Pos);
        static_assert(c_data1Size == 7);
        m_adc1->SQR3 = (ADC_CHANNEL_14 << ADC_SQR3_SQ6_Pos) | 
                       (ADC_CHANNEL_12 << ADC_SQR3_SQ6_Pos) | 
                       (ADC_CHANNEL_10 << ADC_SQR3_SQ5_Pos) | 
                       (ADC_CHANNEL_8 << ADC_SQR3_SQ4_Pos) | 
                       (ADC_CHANNEL_6 << ADC_SQR3_SQ3_Pos) | 
                       (ADC_CHANNEL_2 << ADC_SQR3_SQ2_Pos) | 
                       (ADC_CHANNEL_0 << ADC_SQR3_SQ1_Pos);

        __HAL_RCC_ADC2_CLK_ENABLE();
        __HAL_RCC_ADC2_FORCE_RESET();
        __HAL_RCC_ADC2_RELEASE_RESET();
        m_adc2 = ADC2;
        m_adc2->CR1 = ADC_CR1_SCAN;
        m_adc2->CR2 = ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_ADON;
        RT::stall(72);                              //fixme: wait Tstab=1uS
        m_adc2->CR2 = ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_RSTCAL | ADC_CR2_ADON;
        while (m_adc2->CR2 & ADC_CR2_RSTCAL);
        m_adc2->CR2 = ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_CAL | ADC_CR2_ADON;
        while (m_adc2->CR2 & ADC_CR2_CAL);
        m_adc2->SQR1 = ((c_data1Size - 1) << ADC_SQR1_L_Pos);
        static_assert(c_data1Size == 7);
        m_adc2->SQR3 = (ADC_CHANNEL_15 << ADC_SQR3_SQ6_Pos) | 
                       (ADC_CHANNEL_13 << ADC_SQR3_SQ6_Pos) | 
                       (ADC_CHANNEL_11 << ADC_SQR3_SQ5_Pos) | 
                       (ADC_CHANNEL_9 << ADC_SQR3_SQ4_Pos) | 
                       (ADC_CHANNEL_7 << ADC_SQR3_SQ3_Pos) | 
                       (ADC_CHANNEL_3 << ADC_SQR3_SQ2_Pos) | 
                       (ADC_CHANNEL_1 << ADC_SQR3_SQ1_Pos);

        if (m_switchBsrr)
        {
          __HAL_RCC_ADC3_CLK_ENABLE();
          __HAL_RCC_ADC3_FORCE_RESET();
          __HAL_RCC_ADC3_RELEASE_RESET();
          m_adc3 = ADC3;
          m_adc3->CR1 = ADC_CR1_SCAN;
          m_adc3->CR2 = ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_ADON;
          RT::stall(72);                              //fixme: wait Tstab=1uS
          m_adc3->CR2 = ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_RSTCAL | ADC_CR2_ADON;
          while (m_adc3->CR2 & ADC_CR2_RSTCAL);
          m_adc3->CR2 = ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_CAL | ADC_CR2_ADON;
          while (m_adc3->CR2 & ADC_CR2_CAL);
          m_adc3->SQR1 = ((c_data2Size - 1) << ADC_SQR1_L_Pos);
          static_assert(c_data2Size == 6);
          m_adc3->SQR3 = (ADC_CHANNEL_13 << ADC_SQR3_SQ6_Pos) | 
                         (ADC_CHANNEL_12 << ADC_SQR3_SQ5_Pos) | 
                         (ADC_CHANNEL_11 << ADC_SQR3_SQ4_Pos) | 
                         (ADC_CHANNEL_10 << ADC_SQR3_SQ3_Pos) | 
                         (ADC_CHANNEL_1 << ADC_SQR3_SQ2_Pos) | 
                         (ADC_CHANNEL_0 << ADC_SQR3_SQ1_Pos);
        }

        __HAL_RCC_DMA1_CLK_ENABLE();
        m_dma1 = DMA1;
        m_dma1Rx = DMA1_Channel1;
        m_dma1RxFlags = (DMA_ISR_TEIF1 | DMA_ISR_TCIF1) << (1 - 1) * 4;
        m_handlerDma1Rx.install(DMA1_Channel1_IRQn);
        HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
        m_dma1Rx->CPAR = uint32_t(&m_adc1->DR);
        m_dma1Rx->CMAR = uint32_t(&m_data[0]);

        if (m_switchBsrr)
        {
          __HAL_RCC_DMA2_CLK_ENABLE();
          m_dma2 = DMA2;
          m_dma2Rx = DMA2_Channel5;
          m_dma2RxFlags = (DMA_ISR_TEIF1 | DMA_ISR_TCIF1) << (5 - 1) * 4;
          m_handlerDma2Rx.install(DMA2_Channel4_5_IRQn);
          HAL_NVIC_SetPriority(DMA2_Channel4_5_IRQn, 5, 0);
          HAL_NVIC_EnableIRQ(DMA2_Channel4_5_IRQn);
          m_dma2Rx->CPAR = uint32_t(&m_adc3->DR);
          m_dma2Rx->CMAR = uint32_t(&m_data[c_data1Size * 2]);
        }
#else
#error Unsupported architecture
#endif
      }

      virtual void convert() override
      {
        Irq::SignalingWaiter w(m_handlerDma1Rx);

        if (m_switchBsrr)
          start2();
        else
          start1();

        tm[0] = DWT->CYCCNT;

//        w.wait();
//        wait();
        xTaskNotifyWait(0, 1, NULL, portMAX_DELAY);

        tm[4] = DWT->CYCCNT;

      }
      
    protected:
      void start1()
      {
#if defined(STM32F1)
        m_dma1Rx->CNDTR = c_data1Size;
        m_dma1Rx->CCR = DMA_CCR_PL_0 | DMA_CCR_MSIZE_1 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE | DMA_CCR_EN;
        m_adc1->CR2 = ADC_CR2_SWSTART | ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_ADON;
#endif
      }

      void start2()
      {
#if defined(STM32F1)
        *m_switchBsrr = m_switchSelect2;
        m_dma2Rx->CNDTR = c_data2Size;
//        m_dma2Rx->CCR = DMA_CCR_PL_0 | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC /*| DMA_CCR_TCIE | DMA_CCR_TEIE*/ | DMA_CCR_EN;
        m_dma2Rx->CCR = DMA_CCR_PL_0 | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE | DMA_CCR_EN;
        m_adc3->CR2 = ADC_CR2_SWSTART | ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_ADON;
#endif
      }

      bool handleDma1Rx(IRQn_Type)
      {
        uint32_t const clear = m_dma1->ISR & m_dma1RxFlags;
        if (clear)
        {
          m_dma1->IFCR = clear;
          m_dma1Rx->CCR = 0;

          tm[2] = DWT->CYCCNT;

          m_handlerDma1Rx.signal();    //distinguish success and error

          tm[3] = DWT->CYCCNT;

          return true;
        }

        return false;
      }      

      bool handleDma2Rx(IRQn_Type)
      {
        uint32_t const clear = m_dma2->ISR & m_dma2RxFlags;
        if (clear)
        {
          m_dma2->IFCR = clear;
          m_dma2Rx->CCR = 0;
          *m_switchBsrr = m_switchSelect1;
          start1();
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
      ADC_TypeDef* m_adc1;
      ADC_TypeDef* m_adc2;
      ADC_TypeDef* m_adc3;
      DMA_TypeDef* m_dma1;
      DMA_Channel_TypeDef* m_dma1Rx;
      uint32_t m_dma1RxFlags;
//      Irq::DelegatedHandler<Irq::SemaphoreHandler, AdcImpl, &AdcImpl::handleDma1Rx> m_handlerDma1Rx;
      Irq::DelegatedHandler<Irq::SignalingHandler, AdcImpl, &AdcImpl::handleDma1Rx> m_handlerDma1Rx;
      DMA_TypeDef* m_dma2;
      DMA_Channel_TypeDef* m_dma2Rx;
      uint32_t m_dma2RxFlags;
      Irq::DelegatedHandler<Irq::Handler, AdcImpl, &AdcImpl::handleDma2Rx> m_handlerDma2Rx;
      uint16_t m_data[c_data1Size * 2 + c_data2Size] = {};
    };
  }
}

auto Analog::CreateAdcStm32(GPIO_TypeDef* const switchGPIO, uint16_t const switchPin, bool const switchInvert) -> std::unique_ptr<Adc>
{
  return std::make_unique<AdcImpl>(switchGPIO, switchPin, switchInvert);
}
