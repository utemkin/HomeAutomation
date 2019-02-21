#include <lib/common/handlers.h>
#include <lib/common/utils_stm32.h>

namespace Rt
{
  namespace
  {
    class HiresTimerImpl : public HiresTimer
    {
    public:
      HiresTimerImpl(TIM_TypeDef *const tim, Callback&& callback)
        : m_tim(tim)
        , m_callback(std::move(callback))
        , m_handlerTim(Irq::Handler::Callback::make<HiresTimerImpl, &HiresTimerImpl::handleTim>(*this))
      {
        uint32_t pclk;
        RCC_ClkInitTypeDef clk;
        uint32_t lat;
        HAL_RCC_GetClockConfig(&clk, &lat);
        if (false)
          ;
    #if defined(STM32F1)
    #  ifdef TIM1
        else if (m_tim == TIM1)
        {
          __HAL_RCC_TIM1_CLK_ENABLE();
          __HAL_RCC_TIM1_FORCE_RESET();
          __HAL_RCC_TIM1_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM1_UP_IRQn);
          HAL_NVIC_SetPriority(TIM1_UP_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
        }
    #  endif
    #  ifdef TIM2
        else if (m_tim == TIM2)
        {
          __HAL_RCC_TIM2_CLK_ENABLE();
          __HAL_RCC_TIM2_FORCE_RESET();
          __HAL_RCC_TIM2_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM2_IRQn);
          HAL_NVIC_SetPriority(TIM2_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM2_IRQn);
        }
    #  endif
    #  ifdef TIM3
        else if (m_tim == TIM3)
        {
          __HAL_RCC_TIM3_CLK_ENABLE();
          __HAL_RCC_TIM3_FORCE_RESET();
          __HAL_RCC_TIM3_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM3_IRQn);
          HAL_NVIC_SetPriority(TIM3_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM3_IRQn);
        }
    #  endif
    #  ifdef TIM4
        else if (m_tim == TIM4)
        {
          __HAL_RCC_TIM4_CLK_ENABLE();
          __HAL_RCC_TIM4_FORCE_RESET();
          __HAL_RCC_TIM4_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM4_IRQn);
          HAL_NVIC_SetPriority(TIM4_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM4_IRQn);
        }
    #  endif
    #  ifdef TIM5
        else if (m_tim == TIM5)
        {
          __HAL_RCC_TIM5_CLK_ENABLE();
          __HAL_RCC_TIM5_FORCE_RESET();
          __HAL_RCC_TIM5_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM5_IRQn);
          HAL_NVIC_SetPriority(TIM5_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM5_IRQn);
        }
    #  endif
    #  ifdef TIM6
        else if (m_tim == TIM6)
        {
          __HAL_RCC_TIM6_CLK_ENABLE();
          __HAL_RCC_TIM6_FORCE_RESET();
          __HAL_RCC_TIM6_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM6_IRQn);
          HAL_NVIC_SetPriority(TIM6_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM6_IRQn);
        }
    #  endif
    #  ifdef TIM7
        else if (m_tim == TIM7)
        {
          __HAL_RCC_TIM7_CLK_ENABLE();
          __HAL_RCC_TIM7_FORCE_RESET();
          __HAL_RCC_TIM7_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM7_IRQn);
          HAL_NVIC_SetPriority(TIM7_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM7_IRQn);
        }
    #  endif
    #  ifdef TIM8
        else if (m_tim == TIM8)
        {
          __HAL_RCC_TIM8_CLK_ENABLE();
          __HAL_RCC_TIM8_FORCE_RESET();
          __HAL_RCC_TIM8_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM8_UP_IRQn);
          HAL_NVIC_SetPriority(TIM8_UP_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM8_UP_IRQn);
        }
    #  endif
    #  ifdef TIM9
        else if (m_tim == TIM9)
        {
          __HAL_RCC_TIM9_CLK_ENABLE();
          __HAL_RCC_TIM9_FORCE_RESET();
          __HAL_RCC_TIM9_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM9_IRQn);
          HAL_NVIC_SetPriority(TIM9_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM9_IRQn);
        }
    #  endif
    #  ifdef TIM10
        else if (m_tim == TIM10)
        {
          __HAL_RCC_TIM10_CLK_ENABLE();
          __HAL_RCC_TIM10_FORCE_RESET();
          __HAL_RCC_TIM10_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM10_IRQn);
          HAL_NVIC_SetPriority(TIM10_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM10_IRQn);
        }
    #  endif
    #  ifdef TIM11
        else if (m_tim == TIM11)
        {
          __HAL_RCC_TIM11_CLK_ENABLE();
          __HAL_RCC_TIM11_FORCE_RESET();
          __HAL_RCC_TIM11_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM11_IRQn);
          HAL_NVIC_SetPriority(TIM11_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM11_IRQn);
        }
    #  endif
    #  ifdef TIM12
        else if (m_tim == TIM12)
        {
          __HAL_RCC_TIM12_CLK_ENABLE();
          __HAL_RCC_TIM12_FORCE_RESET();
          __HAL_RCC_TIM12_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM12_IRQn);
          HAL_NVIC_SetPriority(TIM12_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM12_IRQn);
        }
    #  endif
    #  ifdef TIM13
        else if (m_tim == TIM13)
        {
          __HAL_RCC_TIM13_CLK_ENABLE();
          __HAL_RCC_TIM13_FORCE_RESET();
          __HAL_RCC_TIM13_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM13_IRQn);
          HAL_NVIC_SetPriority(TIM13_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM13_IRQn);
        }
    #  endif
    #  ifdef TIM14
        else if (m_tim == TIM14)
        {
          __HAL_RCC_TIM14_CLK_ENABLE();
          __HAL_RCC_TIM14_FORCE_RESET();
          __HAL_RCC_TIM14_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM14_IRQn);
          HAL_NVIC_SetPriority(TIM14_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM14_IRQn);
        }
    #  endif
    #elif defined(STM32F4)
    #  ifdef TIM1
        else if (m_tim == TIM1)
        {
          __HAL_RCC_TIM1_CLK_ENABLE();
          __HAL_RCC_TIM1_FORCE_RESET();
          __HAL_RCC_TIM1_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM1_UP_TIM10_IRQn);
          HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
        }
    #  endif
    #  ifdef TIM2
        else if (m_tim == TIM2)
        {
          __HAL_RCC_TIM2_CLK_ENABLE();
          __HAL_RCC_TIM2_FORCE_RESET();
          __HAL_RCC_TIM2_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM2_IRQn);
          HAL_NVIC_SetPriority(TIM2_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM2_IRQn);
        }
    #  endif
    #  ifdef TIM3
        else if (m_tim == TIM3)
        {
          __HAL_RCC_TIM3_CLK_ENABLE();
          __HAL_RCC_TIM3_FORCE_RESET();
          __HAL_RCC_TIM3_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM3_IRQn);
          HAL_NVIC_SetPriority(TIM3_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM3_IRQn);
        }
    #  endif
    #  ifdef TIM4
        else if (m_tim == TIM4)
        {
          __HAL_RCC_TIM4_CLK_ENABLE();
          __HAL_RCC_TIM4_FORCE_RESET();
          __HAL_RCC_TIM4_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM4_IRQn);
          HAL_NVIC_SetPriority(TIM4_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM4_IRQn);
        }
    #  endif
    #  ifdef TIM5
        else if (m_tim == TIM5)
        {
          __HAL_RCC_TIM5_CLK_ENABLE();
          __HAL_RCC_TIM5_FORCE_RESET();
          __HAL_RCC_TIM5_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM5_IRQn);
          HAL_NVIC_SetPriority(TIM5_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM5_IRQn);
        }
    #  endif
    #  ifdef TIM6
        else if (m_tim == TIM6)
        {
          __HAL_RCC_TIM6_CLK_ENABLE();
          __HAL_RCC_TIM6_FORCE_RESET();
          __HAL_RCC_TIM6_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM6_DAC_IRQn);
          HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
        }
    #  endif
    #  ifdef TIM7
        else if (m_tim == TIM7)
        {
          __HAL_RCC_TIM7_CLK_ENABLE();
          __HAL_RCC_TIM7_FORCE_RESET();
          __HAL_RCC_TIM7_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM7_IRQn);
          HAL_NVIC_SetPriority(TIM7_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM7_IRQn);
        }
    #  endif
    #  ifdef TIM8
        else if (m_tim == TIM8)
        {
          __HAL_RCC_TIM8_CLK_ENABLE();
          __HAL_RCC_TIM8_FORCE_RESET();
          __HAL_RCC_TIM8_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM8_UP_TIM13_IRQn);
          HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
        }
    #  endif
    #  ifdef TIM9
        else if (m_tim == TIM9)
        {
          __HAL_RCC_TIM9_CLK_ENABLE();
          __HAL_RCC_TIM9_FORCE_RESET();
          __HAL_RCC_TIM9_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM1_BRK_TIM9_IRQn);
          HAL_NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
        }
    #  endif
    #  ifdef TIM10
        else if (m_tim == TIM10)
        {
          __HAL_RCC_TIM10_CLK_ENABLE();
          __HAL_RCC_TIM10_FORCE_RESET();
          __HAL_RCC_TIM10_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM1_UP_TIM10_IRQn);
          HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
        }
    #  endif
    #  ifdef TIM11
        else if (m_tim == TIM11)
        {
          __HAL_RCC_TIM11_CLK_ENABLE();
          __HAL_RCC_TIM11_FORCE_RESET();
          __HAL_RCC_TIM11_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK2Freq();
          if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM1_TRG_COM_TIM11_IRQn);
          HAL_NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);
        }
    #  endif
    #  ifdef TIM12
        else if (m_tim == TIM12)
        {
          __HAL_RCC_TIM12_CLK_ENABLE();
          __HAL_RCC_TIM12_FORCE_RESET();
          __HAL_RCC_TIM12_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM8_BRK_TIM12_IRQn);
          HAL_NVIC_SetPriority(TIM8_BRK_TIM12_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM8_BRK_TIM12_IRQn);
        }
    #  endif
    #  ifdef TIM13
        else if (m_tim == TIM13)
        {
          __HAL_RCC_TIM13_CLK_ENABLE();
          __HAL_RCC_TIM13_FORCE_RESET();
          __HAL_RCC_TIM13_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM8_UP_TIM13_IRQn);
          HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
        }
    #  endif
    #  ifdef TIM14
        else if (m_tim == TIM14)
        {
          __HAL_RCC_TIM14_CLK_ENABLE();
          __HAL_RCC_TIM14_FORCE_RESET();
          __HAL_RCC_TIM14_RELEASE_RESET();
          pclk = HAL_RCC_GetPCLK1Freq();
          if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
            pclk <<= 1;
          m_handlerTim.install(TIM8_TRG_COM_TIM14_IRQn);
          HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, 14, 0);
          HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
        }
    #  endif
    #else
    #  error Unsupported architecture
    #endif
        else
        {
          Rt::fatal();  //fixme
        }
        
        m_pclk = pclk;
      }

      virtual void start(uint32_t const hz) override
      {
        uint32_t div = m_pclk / hz;
        uint32_t psc = 1;
        while (div > 0x10000)
        {
          div >>= 1;
          psc <<= 1;
        }

        m_tim->DIER = TIM_DIER_UIE;
        m_tim->PSC = psc - 1;
        m_tim->CR1 = TIM_CR1_URS;
        m_tim->EGR = TIM_EGR_UG;
        m_tim->ARR = div - 1;
        m_tim->CR1 = TIM_CR1_URS | TIM_CR1_CEN;
      }

    protected:
      bool handleTim(Hal::Irq)
      {
        auto const sr = m_tim->SR;
        if (sr & TIM_SR_UIF)
        {
          m_tim->SR = sr & ~TIM_SR_UIF;
          m_callback();
          return true;
        }

        return false;
      }

    protected:
      TIM_TypeDef* const m_tim;
      Callback const m_callback;
      Irq::Handler m_handlerTim;
      uint32_t m_pclk;
    };
  }

  void fatal()
  {
    for (;;);     //fixme
  }

#if 1
  // real measured delay min(24, [cycles, cycles + 7])
  void ATTR_SUPER_OPTIMIZE stall(const unsigned cycles)
  {
    const decltype(DWT->CYCCNT)* const cyccnt = &DWT->CYCCNT;
    const unsigned initial = *cyccnt - 23;
    while (*cyccnt - initial <= cycles);
  }
#else
  // real measured delay min(31, [cycles, cycles + 3])
  void ATTR_SUPER_OPTIMIZE stall(const unsigned cycles)
  {
    const decltype(DWT->CYCCNT)* const cyccnt = &DWT->CYCCNT;
    const unsigned initial = *cyccnt - 34;
    asm volatile(
    "   .align  3                 \n\t"
    "loop%=:                      \n\t"
    "   ldr     r3, [%[cyccnt]]   \n\t"
    "   subs    r3, %[initial]    \n\t"
    "   subs    r3, %[cycles]     \n\t"
    "   bls     loop%=            \n\t"
    "   cmp     r3, #10           \n\t"
    "   bge     exit%=            \n\t"
    "   cmp     r3, #8            \n\t"
    "   bge     exit%=            \n\t"
    "   cmp     r3, #6            \n\t"
    "   bge     exit%=            \n\t"
    "   cmp     r3, #4            \n\t"
    "   bge     exit%=            \n\t"
    "   cmp     r3, #2            \n\t"
    "   bge     exit%=            \n\t"
    "   nop                       \n\t"
    "   b       exit%=            \n\t"
    "   .align  2                 \n\t"
    "exit%=:                      \n\t"
    "                               " : : [cycles] "r" (cycles), [cyccnt] "r" (cyccnt), [initial] "r" (initial) : "cc", "r3"
    );
  }
#endif
/*
  portDISABLE_INTERRUPTS();
  DWT->CYCCNT = 0;
  for (int i = 0; i < 100; ++i)
  {
    unsigned i1 = DWT->CYCCNT;
    RT::stall(i);
    unsigned i2 = DWT->CYCCNT;
    printf("i=%u, CYCCNT=%u\n", i, i2 - i1);
  }
  portENABLE_INTERRUPTS();
*/

  uint32_t getUnique()
  {
    uint32_t uid[3];
    HAL_GetUID(uid);
    return Tools::CRC32::calculate((const uint8_t*)&uid, sizeof(uid));
  }
}

auto Rt::CreateHiresTimer(TIM_TypeDef *tim, HiresTimer::Callback&& callback) -> std::unique_ptr<HiresTimer>
{
  return std::make_unique<HiresTimerImpl>(tim, std::move(callback));
}
