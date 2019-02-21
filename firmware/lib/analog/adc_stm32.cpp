#include <lib/analog/adc_stm32.h>
#include <lib/common/hal.h>
#include <lib/common/handlers.h>
#include <lib/common/utils.h>
#include <limits>

namespace Analog
{
  namespace
  {
#if defined(STM32F1)
    class AdcImpl : public Adc
    {
    public:
      AdcImpl(const Pin::Def& select2, Callback&& callback)
        : m_select2(select2)
        , m_callback(std::move(callback))
        , m_handlerDma1(Irq::Handler::Callback::make<AdcImpl, &AdcImpl::handleDma1>(*this))
        , m_handlerDma2(Irq::Handler::Callback::make<AdcImpl, &AdcImpl::handleDma2>(*this))
      {
        if (Hal::DmaLine::create(m_dma1, Hal::DmaLine::Setup{
          .resource = 
            {
            .controller = 1,  //fixme
            .line = 1,        //fixme
            },
          .config = Hal::DmaLine::c_config_PRIO_LOW | Hal::DmaLine::c_config_M16 | Hal::DmaLine::c_config_P16 | Hal::DmaLine::c_config_MINC | Hal::DmaLine::c_config_P2M,
          .interruptFlags = Hal::DmaLine::c_flags_TC | Hal::DmaLine::c_flags_E,
        }) != Hal::Status::Success)
          Rt::fatal();  //fixme

        if (Hal::DmaLine::create(m_dma2, Hal::DmaLine::Setup{
          .resource = 
            {
            .controller = 2,  //fixme
            .line = 5,        //fixme
            },
          .config = Hal::DmaLine::c_config_PRIO_LOW | Hal::DmaLine::c_config_M16 | Hal::DmaLine::c_config_P16 | Hal::DmaLine::c_config_MINC | Hal::DmaLine::c_config_P2M,
          .interruptFlags = Hal::DmaLine::c_flags_TC | Hal::DmaLine::c_flags_E,
        }) != Hal::Status::Success)
          Rt::fatal();  //fixme

        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_ADC1_FORCE_RESET();
        __HAL_RCC_ADC1_RELEASE_RESET();
        m_adc1 = ADC1;
        m_adc1->CR1 = mstd::bits_at<ADC_CR1_DUALMOD_Pos>(0b0110) | ADC_CR1_SCAN;
        m_adc1->CR2 = ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_DMA | ADC_CR2_ADON;
        Rt::stall(HAL_RCC_GetHCLKFreq() / 1000000);
        m_adc1->CR2 = ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_DMA | ADC_CR2_RSTCAL | ADC_CR2_ADON;
        while (m_adc1->CR2 & ADC_CR2_RSTCAL);
        m_adc1->CR2 = ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_DMA | ADC_CR2_CAL | ADC_CR2_ADON;
        while (m_adc1->CR2 & ADC_CR2_CAL);
        m_adc1->SQR1 = mstd::bits_at<ADC_SQR1_L_Pos>(c_data1Size - 1);
        static_assert(c_data1Size == 7);
        m_adc1->SQR2 = mstd::bits_at<ADC_SQR2_SQ7_Pos>(ADC_CHANNEL_14);
        m_adc1->SQR3 = mstd::bits_at<ADC_SQR3_SQ6_Pos>(ADC_CHANNEL_12) | 
                       mstd::bits_at<ADC_SQR3_SQ5_Pos>(ADC_CHANNEL_10) | 
                       mstd::bits_at<ADC_SQR3_SQ4_Pos>(ADC_CHANNEL_8) | 
                       mstd::bits_at<ADC_SQR3_SQ3_Pos>(ADC_CHANNEL_6) | 
                       mstd::bits_at<ADC_SQR3_SQ2_Pos>(ADC_CHANNEL_2) | 
                       mstd::bits_at<ADC_SQR3_SQ1_Pos>(ADC_CHANNEL_0);

        __HAL_RCC_ADC2_CLK_ENABLE();
        __HAL_RCC_ADC2_FORCE_RESET();
        __HAL_RCC_ADC2_RELEASE_RESET();
        m_adc2 = ADC2;
        m_adc2->CR1 = ADC_CR1_SCAN;
        m_adc2->CR2 = ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_ADON;
        Rt::stall(HAL_RCC_GetHCLKFreq() / 1000000);
        m_adc2->CR2 = ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_RSTCAL | ADC_CR2_ADON;
        while (m_adc2->CR2 & ADC_CR2_RSTCAL);
        m_adc2->CR2 = ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_CAL | ADC_CR2_ADON;
        while (m_adc2->CR2 & ADC_CR2_CAL);
        m_adc2->SQR1 = mstd::bits_at<ADC_SQR1_L_Pos>(c_data1Size - 1);
        static_assert(c_data1Size == 7);
        m_adc2->SQR2 = mstd::bits_at<ADC_SQR2_SQ7_Pos>(ADC_CHANNEL_15);
        m_adc2->SQR3 = mstd::bits_at<ADC_SQR3_SQ6_Pos>(ADC_CHANNEL_13) | 
                       mstd::bits_at<ADC_SQR3_SQ5_Pos>(ADC_CHANNEL_11) | 
                       mstd::bits_at<ADC_SQR3_SQ4_Pos>(ADC_CHANNEL_9) | 
                       mstd::bits_at<ADC_SQR3_SQ3_Pos>(ADC_CHANNEL_7) | 
                       mstd::bits_at<ADC_SQR3_SQ2_Pos>(ADC_CHANNEL_3) | 
                       mstd::bits_at<ADC_SQR3_SQ1_Pos>(ADC_CHANNEL_1);

        if (m_select2)
        {
          __HAL_RCC_ADC3_CLK_ENABLE();
          __HAL_RCC_ADC3_FORCE_RESET();
          __HAL_RCC_ADC3_RELEASE_RESET();
          m_adc3 = ADC3;
          m_adc3->CR1 = ADC_CR1_SCAN;
          m_adc3->CR2 = ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_DMA | ADC_CR2_ADON;
          Rt::stall(HAL_RCC_GetHCLKFreq() / 1000000);
          m_adc3->CR2 = ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_DMA | ADC_CR2_RSTCAL | ADC_CR2_ADON;
          while (m_adc3->CR2 & ADC_CR2_RSTCAL);
          m_adc3->CR2 = ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_DMA | ADC_CR2_CAL | ADC_CR2_ADON;
          while (m_adc3->CR2 & ADC_CR2_CAL);
          m_adc3->SQR1 = mstd::bits_at<ADC_SQR1_L_Pos>(c_data1Size - 1);
          static_assert(c_data2Size == 6);
          m_adc3->SQR3 = mstd::bits_at<ADC_SQR3_SQ6_Pos>(ADC_CHANNEL_13) | 
                         mstd::bits_at<ADC_SQR3_SQ5_Pos>(ADC_CHANNEL_12) | 
                         mstd::bits_at<ADC_SQR3_SQ4_Pos>(ADC_CHANNEL_11) | 
                         mstd::bits_at<ADC_SQR3_SQ3_Pos>(ADC_CHANNEL_10) | 
                         mstd::bits_at<ADC_SQR3_SQ2_Pos>(ADC_CHANNEL_1) | 
                         mstd::bits_at<ADC_SQR3_SQ1_Pos>(ADC_CHANNEL_0);
        }

        m_handlerDma1.install(m_dma1->irq());
        HAL_NVIC_SetPriority(m_dma1->irq(), 5, 0);
        HAL_NVIC_EnableIRQ(m_dma1->irq());

        m_dma1->setNDTR(c_data1Size);
        m_dma1->setPAR(uint32_t(&m_adc1->DR));
        m_dma1->setMAR(uint32_t(&m_data[0]));

        if (m_select2)
        {
          m_handlerDma2.install(m_dma2->irq());
          HAL_NVIC_SetPriority(m_dma2->irq(), 5, 0);
          HAL_NVIC_EnableIRQ(m_dma2->irq());

          m_dma2->setNDTR(c_data2Size);
          m_dma2->setPAR(uint32_t(&m_adc3->DR));
          m_dma2->setMAR(uint32_t(&m_data[c_data1Size * 2]));
        }
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
        m_dma1->start();
        m_adc1->CR2 = ADC_CR2_SWSTART | ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_DMA | ADC_CR2_ADON;
      }

      void start2()
      {
        m_select2.toActive();
        m_dma2->start();
        m_adc3->CR2 = ADC_CR2_SWSTART | ADC_CR2_EXTTRIG | mstd::bits_at<ADC_CR2_EXTSEL_Pos>(0b111) | ADC_CR2_DMA | ADC_CR2_ADON;
      }

      bool handleDma1(Hal::Irq)
      {
        auto const flags = m_dma1->flagsGetAndClear();
        if (flags)
        {
          m_dma1->stop();
          m_callback(true);    //distinguish success and error
          return true;
        }

        return false;
      }      

      bool handleDma2(Hal::Irq)
      {
        auto const flags = m_dma2->flagsGetAndClear();
        if (flags)
        {
          m_dma2->stop();
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
      Irq::Handler m_handlerDma1;
      Irq::Handler m_handlerDma2;
      std::unique_ptr<Hal::DmaLine> m_dma1;
      std::unique_ptr<Hal::DmaLine> m_dma2;
      ADC_TypeDef* m_adc1;
      ADC_TypeDef* m_adc2;
      ADC_TypeDef* m_adc3;
      volatile uint16_t m_data[c_data1Size * 2 + c_data2Size] = {};
    };
#elif defined(STM32F4)
    class AdcImpl : public Adc
    {
    public:
      AdcImpl(Callback&& callback)
        : m_callback(std::move(callback))
        , m_handlerAdc(Irq::Handler::Callback::make<AdcImpl, &AdcImpl::handleAdc>(*this))
        , m_adc1(ADC1)
        , m_adc2(ADC2)
        , m_adc3(ADC3)
        , m_adcCommon(ADC123_COMMON)
      {
        if (Hal::DmaLine::create(m_dma, Hal::DmaLine::Setup{
          .resource = 
            {
            .controller = 2,  //fixme
            .line = 0,        //fixme
            },
          .channel = 0,
          .config = Hal::DmaLine::c_config_PRIO_LOW | Hal::DmaLine::c_config_M16 | Hal::DmaLine::c_config_P16 | Hal::DmaLine::c_config_MINC | Hal::DmaLine::c_config_P2M,
          .fifoControl = Hal::DmaLine::c_fifoControl_DMDIS | Hal::DmaLine::c_fifoControl_THRESH_2DIV4,
        }) != Hal::Status::Success)
          Rt::fatal();  //fixme

        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_ADC2_CLK_ENABLE();
        __HAL_RCC_ADC3_CLK_ENABLE();
        __HAL_RCC_ADC_FORCE_RESET();
        __HAL_RCC_ADC_RELEASE_RESET();

        m_adc1->CR2 = ADC_CR2_ADON;
        m_adc1->CR1 = ADC_CR1_OVRIE | ADC_CR1_SCAN;
        m_adc1->SMPR1 = mstd::bits_at<ADC_SMPR1_SMP18_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP17_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP16_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP15_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP14_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP13_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP12_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP11_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP10_Pos>(0b001);
        m_adc1->SMPR2 = mstd::bits_at<ADC_SMPR2_SMP9_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP8_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP7_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP6_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP5_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP4_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP3_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP2_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP1_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP0_Pos>(0b001);
        static_assert(c_dataSize == 7);
        m_adc1->SQR1 = mstd::bits_at<ADC_SQR1_L_Pos>(c_dataSize - 1);
        m_adc1->SQR2 = mstd::bits_at<ADC_SQR2_SQ7_Pos>(ADC_CHANNEL_8);
        m_adc1->SQR3 = mstd::bits_at<ADC_SQR3_SQ6_Pos>(ADC_CHANNEL_7) | 
                       mstd::bits_at<ADC_SQR3_SQ5_Pos>(ADC_CHANNEL_6) | 
                       mstd::bits_at<ADC_SQR3_SQ4_Pos>(ADC_CHANNEL_3) | 
                       mstd::bits_at<ADC_SQR3_SQ3_Pos>(ADC_CHANNEL_2) | 
                       mstd::bits_at<ADC_SQR3_SQ2_Pos>(ADC_CHANNEL_1) | 
                       mstd::bits_at<ADC_SQR3_SQ1_Pos>(ADC_CHANNEL_0);

        m_adc2->CR2 = ADC_CR2_ADON;
        m_adc2->CR1 = ADC_CR1_OVRIE | ADC_CR1_SCAN;
        m_adc2->SMPR1 = mstd::bits_at<ADC_SMPR1_SMP18_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP17_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP16_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP15_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP14_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP13_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP12_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP11_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP10_Pos>(0b001);
        m_adc2->SMPR2 = mstd::bits_at<ADC_SMPR2_SMP9_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP8_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP7_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP6_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP5_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP4_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP3_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP2_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP1_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP0_Pos>(0b001);
        static_assert(c_dataSize == 7);
        m_adc2->SQR1 = mstd::bits_at<ADC_SQR1_L_Pos>(c_dataSize - 1);
        m_adc2->SQR2 = mstd::bits_at<ADC_SQR2_SQ7_Pos>(ADC_CHANNEL_15);
        m_adc2->SQR3 = mstd::bits_at<ADC_SQR3_SQ6_Pos>(ADC_CHANNEL_14) | 
                       mstd::bits_at<ADC_SQR3_SQ5_Pos>(ADC_CHANNEL_13) | 
                       mstd::bits_at<ADC_SQR3_SQ4_Pos>(ADC_CHANNEL_12) | 
                       mstd::bits_at<ADC_SQR3_SQ3_Pos>(ADC_CHANNEL_11) | 
                       mstd::bits_at<ADC_SQR3_SQ2_Pos>(ADC_CHANNEL_10) | 
                       mstd::bits_at<ADC_SQR3_SQ1_Pos>(ADC_CHANNEL_9);

        m_adc3->CR2 = ADC_CR2_ADON;
        m_adc3->CR1 = ADC_CR1_OVRIE | ADC_CR1_SCAN | ADC_CR1_EOCIE;
        m_adc3->SMPR1 = mstd::bits_at<ADC_SMPR1_SMP18_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP17_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP16_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP15_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP14_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP13_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP12_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP11_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR1_SMP10_Pos>(0b001);
        m_adc3->SMPR2 = mstd::bits_at<ADC_SMPR2_SMP9_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP8_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP7_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP6_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP5_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP4_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP3_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP2_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP1_Pos>(0b001) |
                        mstd::bits_at<ADC_SMPR2_SMP0_Pos>(0b001);
        static_assert(c_dataSize == 7);
        m_adc3->SQR1 = mstd::bits_at<ADC_SQR1_L_Pos>(c_dataSize - 1);
        m_adc3->SQR2 = mstd::bits_at<ADC_SQR2_SQ7_Pos>(ADC_CHANNEL_15);
        m_adc3->SQR3 = mstd::bits_at<ADC_SQR3_SQ6_Pos>(ADC_CHANNEL_14) | 
                       mstd::bits_at<ADC_SQR3_SQ5_Pos>(ADC_CHANNEL_8) | 
                       mstd::bits_at<ADC_SQR3_SQ4_Pos>(ADC_CHANNEL_7) | 
                       mstd::bits_at<ADC_SQR3_SQ3_Pos>(ADC_CHANNEL_6) | 
                       mstd::bits_at<ADC_SQR3_SQ2_Pos>(ADC_CHANNEL_5) | 
                       mstd::bits_at<ADC_SQR3_SQ1_Pos>(ADC_CHANNEL_4);

        uint32_t pclk = HAL_RCC_GetPCLK2Freq();
        uint32_t pre;
        for (pre = 0; pre < 4; ++pre)
          if (pclk / ((pre + 1) * 2) <= 36000000)
            break;

        if (pclk / ((pre + 1) * 2) > 36000000)
          Rt::fatal();  //fixme

        m_adcCommon->CCR = mstd::bits_at<ADC_CCR_ADCPRE_Pos>(pre) | mstd::bits_at<ADC_CCR_DMA_Pos>(0b01) | mstd::bits_at<ADC_CCR_MULTI_Pos>(0b10110);

        Rt::stall(HAL_RCC_GetHCLKFreq() / 1000000 * 3);

        m_handlerAdc.install(ADC_IRQn);
        HAL_NVIC_SetPriority(ADC_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(ADC_IRQn);

        m_dma->setNDTR(c_dataSize * 3 - 1);
        m_dma->setPAR(uint32_t(&m_adcCommon->CDR));
        m_dma->setMAR(uint32_t(&m_data));
      }

      virtual const volatile uint16_t* channel(size_t num) const override
      {
        auto const count = c_dataSize * 3;
        return num < count ? m_data + num : nullptr;
      }

      virtual void start() override
      {
        m_dma->start();
        auto const CCR = m_adcCommon->CCR;
        m_adcCommon->CCR = CCR & ~ADC_CCR_DMA_Msk;
        m_adcCommon->CCR = CCR;
        m_adc1->CR2 = ADC_CR2_SWSTART | ADC_CR2_ADON;
      }

    protected:
      bool handleAdc(Hal::Irq)
      {
        auto const CSR = m_adcCommon->CSR;
        if (CSR & (ADC_CSR_OVR1 | ADC_CSR_OVR2 | ADC_CSR_OVR3 | ADC_CSR_EOC3))
        {
          auto const flags = m_dma->flagsGetAndClear(Hal::DmaLine::c_flags_TC | Hal::DmaLine::c_flags_E);
          if ((CSR & (ADC_CSR_OVR1 | ADC_CSR_OVR2 | ADC_CSR_OVR3 | ADC_CSR_EOC1 | ADC_CSR_EOC2)) || flags != Hal::DmaLine::c_flags_TC)
          {
            m_dma->stop();
            m_adc1->SR = 0;
            m_adc2->SR = 0;
            m_adc3->SR = 0;
            m_callback(false);
            return true;
          }

          m_data[std::extent<decltype(m_data)>::value - 1] = m_adcCommon->CDR;
          m_dma->stop();
          m_callback(true);
          return true;
        }

        return false;
      }

    protected:
      constexpr static size_t c_dataSize = 7;
      Callback const m_callback;
      Irq::Handler m_handlerAdc;
      ADC_TypeDef* const m_adc1;
      ADC_TypeDef* const m_adc2;
      ADC_TypeDef* const m_adc3;
      ADC_Common_TypeDef* const m_adcCommon;
      std::unique_ptr<Hal::DmaLine> m_dma;
      // [0]  = ADC123_IN0  = PA0
      // [1]  = ADC12_IN9   = PB1
      // [2]  = ADC3_IN4    = PF6
      // [3]  = ADC123_IN1  = PA1
      // [4]  = ADC123_IN10 = PC0
      // [5]  = ADC3_IN5    = PF7
      // [6]  = ADC123_IN2  = PA2
      // [7]  = ADC123_IN11 = PC1
      // [8]  = ADC3_IN6    = PF8
      // [9]  = ADC123_IN3  = PA3
      // [10] = ADC123_IN12 = PC2
      // [11] = ADC3_IN7    = PF9
      // [12] = ADC123_IN6  = PA6
      // [13] = ADC123_IN13 = PC3
      // [14] = ADC3_IN8    = PF10
      // [15] = ADC123_IN7  = PA7
      // [16] = ADC123_IN14 = PC4
      // [17] = ADC3_IN14   = PF4
      // [18] = ADC123_IN8  = PB0
      // [19] = ADC123_IN15 = PC5
      // [20] = ADC3_IN15   = PF5
      volatile uint16_t m_data[c_dataSize * 3] = {};
    };
#else
#error Unsupported architecture
#endif
  }
}

#if defined(STM32F1)
auto Analog::CreateAdcStm32(const Pin::Def& select2, Adc::Callback&& callback) -> std::unique_ptr<Adc>
{
  return std::make_unique<AdcImpl>(select2, std::move(callback));
}
#elif defined(STM32F4)
auto Analog::CreateAdcStm32(Adc::Callback&& callback) -> std::unique_ptr<Adc>
{
  return std::make_unique<AdcImpl>(std::move(callback));
}
#else
#error Unsupported architecture
#endif
