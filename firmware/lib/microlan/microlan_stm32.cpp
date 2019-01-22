#include <lib/microlan/microlan_stm32.h>

namespace MicroLan
{
  TimingGenerator::TimingGenerator()
    : m_handlerTim(Irq::Handler::Callback::make<TimingGenerator, &TimingGenerator::handleTimIrq>(*this))
#if defined(STM32F1)
    , m_tim(TIM2)                   //fixme
    , m_tim_CC_Out0(&m_tim->CCR2)   //fixme
    , m_tim_CC_Out1(&m_tim->CCR4)   //fixme
    , m_tim_CC_Sample(&m_tim->CCR1) //fixme
    , m_IRQn(TIM2_IRQn)             //fixme
    , m_dmaOut(DMA1_Channel7, HAL::DmaLine::c_config_PRIO_LOW | HAL::DmaLine::c_config_M32 | HAL::DmaLine::c_config_P32 | HAL::DmaLine::c_config_MINC | HAL::DmaLine::c_config_M2P, 0)   //fixme
    , m_dmaIn(DMA1_Channel5, HAL::DmaLine::c_config_PRIO_LOW | HAL::DmaLine::c_config_M32 | HAL::DmaLine::c_config_P32 | HAL::DmaLine::c_config_MINC | HAL::DmaLine::c_config_P2M, 0)    //fixme
#elif defined(STM32F4)
    , m_tim(TIM1)                   //fixme
    , m_tim_CC_Out0(&m_tim->CCR2)   //fixme
    , m_tim_CC_Out1(&m_tim->CCR3)   //fixme
    , m_tim_CC_Sample(&m_tim->CCR1) //fixme
    , m_IRQn(TIM1_UP_TIM10_IRQn)    //fixme
    , m_dmaOut(DMA2_Stream6, 0, HAL::DmaLine::c_config_PRIO_LOW | HAL::DmaLine::c_config_M32 | HAL::DmaLine::c_config_P32 | HAL::DmaLine::c_config_MINC | HAL::DmaLine::c_config_M2P, 0, 0)   //fixme
    , m_dmaIn(DMA2_Stream1, 6, HAL::DmaLine::c_config_PRIO_LOW | HAL::DmaLine::c_config_M32 | HAL::DmaLine::c_config_P32 | HAL::DmaLine::c_config_MINC | HAL::DmaLine::c_config_P2M, 0, 0)    //fixme
#else
#error Unsupported architecture
#endif
  {
    init();
    start();
  }

  bool TimingGenerator::touch(const Timings& timings, const Data& data)
  {
    m_dmaIn.setPAR(data.inAddr);
    m_dmaIn.start();

    m_out[0] = data.out0;
    m_out[1] = data.out1;

    m_dmaOut.setPAR(data.outAddr);
    m_dmaOut.start();

    m_tim->ARR = timings.ticksTotal;
    *m_tim_CC_Out0 = timings.ticksOut0;
    *m_tim_CC_Out1 = timings.ticksOut1;
    *m_tim_CC_Sample = timings.ticksSample;
    m_tim->CR1 |= TIM_CR1_CEN;

    m_handlerTim.wait();

    __DMB();
    return (!!(m_in & data.inMask)) ^ data.inInvert;
  }

  bool TimingGenerator::handleTimIrq(IRQn_Type /*IRQn*/)
  {
    if (m_tim->SR & TIM_IT_UPDATE)
    {
      m_tim->SR = ~TIM_IT_UPDATE;
      m_handlerTim.signal();
      return true;
    }

    return false;
  }

  void TimingGenerator::init()
  {
    __HAL_RCC_TIM1_CLK_ENABLE();  //fixme
    m_handlerTim.install(m_IRQn);
    HAL_NVIC_SetPriority(m_IRQn, 5, 0);

    uint32_t pclk = HAL_RCC_GetPCLK1Freq();
    RCC_ClkInitTypeDef clk;
    uint32_t lat;
    HAL_RCC_GetClockConfig(&clk, &lat);
    if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
      pclk <<= 1;
    uint16_t prescale = 1;
    while (pclk / 1000 > std::numeric_limits<uint16_t>::max())
    {
      prescale <<= 1;
      pclk >>= 1;
    }
    m_prescale = prescale;
    m_unitClock = pclk;
  }

  void TimingGenerator::start()
  {
    __HAL_RCC_TIM1_FORCE_RESET();   //fixme
    __HAL_RCC_TIM1_RELEASE_RESET(); //fixme

    m_dmaIn.setMAR(uint32_t(&m_in));
    m_dmaIn.setNDTR(1);
    m_dmaOut.setMAR(uint32_t(&m_out));
    m_dmaOut.setNDTR(2);

//      m_tim->DIER = TIM_DIER_CC4DE | TIM_DIER_CC2DE | TIM_DIER_CC1DE | TIM_DIER_UIE;  //fixme
    m_tim->DIER = TIM_DIER_CC3DE | TIM_DIER_CC2DE | TIM_DIER_CC1DE | TIM_DIER_UIE;  //fixme
    m_tim->PSC = m_prescale - 1;
    m_tim->CR1 = TIM_CR1_OPM | TIM_CR1_URS;
    m_tim->EGR = TIM_EGR_UG;

    HAL_NVIC_EnableIRQ(m_IRQn);
  }

  void TimingGenerator::stop()
  {
    HAL_NVIC_DisableIRQ(m_IRQn);
    m_dmaIn.stop();
    m_dmaOut.stop();
    m_tim->CR1 = 0;
  }

  uint16_t TimingGenerator::toTicks(unsigned const intervalNs) const
  {
    return Bus::toUnit(intervalNs, m_unitClock) + 1;
  }
}
