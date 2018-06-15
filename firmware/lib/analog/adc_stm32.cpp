#include <lib/analog/adc_stm32.h>
#include <lib/common/handlers.h>
#include <limits>

namespace Analog
{
  namespace
  {
    class AdcImpl : public Adc
    {
    public:
      AdcImpl(const Pin::Def& select2, Callback&& callback)
        : m_select2(select2)
        , m_callback(std::move(callback))
        , m_handlerDma1Rx(Irq::Handler::Callback::make<AdcImpl, &AdcImpl::handleDma1Rx>(*this))
        , m_handlerDma2Rx(Irq::Handler::Callback::make<AdcImpl, &AdcImpl::handleDma2Rx>(*this))
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

        if (m_select2)
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

        if (m_select2)
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

      virtual const volatile uint16_t* channel(size_t num) const override
      {
        auto const count = m_select2 ? c_data1Size * 2 + c_data2Size : c_data1Size * 2;
        return num < count ? m_data + num : nullptr;
      }

      virtual void start() override
      {
        if (m_select2)
          start2();
        else
          start1();
      }
      
    protected:
      void start1()
      {
        m_dma1Rx->CNDTR = c_data1Size;
        m_dma1Rx->CCR = DMA_CCR_PL_0 | DMA_CCR_MSIZE_1 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE | DMA_CCR_EN;
        m_adc1->CR2 = ADC_CR2_SWSTART | ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_ADON;
      }

      void start2()
      {
        m_select2.toActive();
        m_dma2Rx->CNDTR = c_data2Size;
        m_dma2Rx->CCR = DMA_CCR_PL_0 | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE | DMA_CCR_EN;
        m_adc3->CR2 = ADC_CR2_SWSTART | ADC_CR2_EXTTRIG | (7 << ADC_CR2_EXTSEL_Pos) | ADC_CR2_DMA | ADC_CR2_ADON;
      }

      bool handleDma1Rx(IRQn_Type)
      {
        uint32_t const clear = m_dma1->ISR & m_dma1RxFlags;
        if (clear)
        {
          m_dma1->IFCR = clear;
          m_dma1Rx->CCR = 0;
          m_callback();    //distinguish success and error
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
          m_select2.toPassive();
          start1();
          return true;
        }

        return false;
      }      

    protected:
      constexpr static size_t c_data1Size = 7;
      constexpr static size_t c_data2Size = 6;
      Pin::Out const m_select2;
      Callback const m_callback;
      Irq::Handler m_handlerDma1Rx;
      Irq::Handler m_handlerDma2Rx;
      ADC_TypeDef* m_adc1;
      ADC_TypeDef* m_adc2;
      ADC_TypeDef* m_adc3;
      DMA_TypeDef* m_dma1;
      DMA_Channel_TypeDef* m_dma1Rx;
      uint32_t m_dma1RxFlags;
      DMA_TypeDef* m_dma2;
      DMA_Channel_TypeDef* m_dma2Rx;
      uint32_t m_dma2RxFlags;
      volatile uint16_t m_data[c_data1Size * 2 + c_data2Size] = {};
    };
  }
}

auto Analog::CreateAdcStm32(const Pin::Def& select2, Adc::Callback&& callback) -> std::unique_ptr<Adc>
{
  return std::make_unique<AdcImpl>(select2, std::move(callback));
}
