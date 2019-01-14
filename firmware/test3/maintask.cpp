#include <lib/common/utils.h>
#include <lib/common/handlers.h>
#include <lib/analog/adc_stm32.h>
#include <lib/microlan/microlan.h>
#include <lib/common/hal.h>
#include <limits>

class TimingGenerator
{
public:
  TimingGenerator()
    : m_tim(TIM2)               //fixme
    , m_IRQn(TIM2_IRQn)         //fixme
    , m_handlerTim(Irq::Handler::Callback::make<TimingGenerator, &TimingGenerator::handleTimIrq>(*this))
    , m_dmaIn(DMA1_Channel5)    //fixme
    , m_dmaOut(DMA1_Channel7)   //fixme
  {
    init();
    start();
  }

protected:
  bool handleTimIrq(IRQn_Type /*IRQn*/)
  {
    if (m_tim->SR & TIM_IT_UPDATE)
    {
      m_tim->SR = ~TIM_IT_UPDATE;
      m_handlerTim.signal();
      return true;
    }

    return false;
  }

protected:
  TIM_TypeDef* const m_tim;
  IRQn_Type const m_IRQn;
  Irq::SemaphoreHandler m_handlerTim;
  DMA_Channel_TypeDef* const m_dmaIn;
  DMA_Channel_TypeDef* const m_dmaOut;
  uint16_t m_prescale;
  volatile uint32_t m_in;
  volatile uint32_t m_out[2];
  
//  static constexpr unsigned c_prescale = 2;
//  static constexpr unsigned c_CLK = TIMCLK / c_prescale;

  struct Timings
  {
    uint16_t UnitsTotal;
    uint16_t Unitsout0;
    uint16_t Unitsout1;
    uint16_t UnitsSample;
  };

  struct Data
  {
    uint32_t outAddr;
    uint32_t out1;
    uint32_t out0;
    uint32_t inAddr;
    uint16_t inMask;
    bool inInvert;
  };

protected:

  // Must do permanent initializations like turning on peripheral clocks, setting interrupt priorities, installing interrupt handlers
  void init()
  {
    __HAL_RCC_TIM2_CLK_ENABLE();  //fixme
    __HAL_RCC_DMA1_CLK_ENABLE();  //fixme
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
  }

  // Must put peripheral to normal operational state either after init() or stop()
  void start()
  {
    RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;    //fixme
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;   //fixme

    m_dmaIn->CMAR = uint32_t(&m_in);
    m_dmaOut->CMAR = uint32_t(&m_out);

    m_tim->DIER = TIM_DIER_CC4DE | TIM_DIER_CC2DE | TIM_DIER_CC1DE | TIM_DIER_UIE;
    m_tim->PSC = m_prescale - 1;
    m_tim->CR1 = TIM_CR1_OPM | TIM_CR1_URS;
    m_tim->EGR = TIM_EGR_UG;

    HAL_NVIC_EnableIRQ(m_IRQn);
  }

  // Must stop all peripheral activities (even if called the middle of operation) and prevent further interrupt|DMA requests
  void stop()
  {
    HAL_NVIC_DisableIRQ(m_IRQn);
    m_dmaIn->CCR = 0;
    m_dmaOut->CCR = 0;
    m_tim->CR1 = 0;
  }

  bool touch(const Timings& timings, const Data& data)
  {
    m_dmaIn->CCR = 0;
    m_dmaIn->CPAR = data.inAddr;
    m_dmaIn->CNDTR = 1;
    m_dmaIn->CCR = DMA_CCR_PL_1 | DMA_CCR_PL_0 | DMA_CCR_MSIZE_1 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_EN;

    m_out[0] = data.out0;
    m_out[1] = data.out1;
    __DSB();

    m_dmaOut->CCR = 0;
    m_dmaOut->CPAR = data.outAddr;
    m_dmaOut->CNDTR = 2;
    m_dmaOut->CCR = DMA_CCR_PL_1 | DMA_CCR_PL_0 | DMA_CCR_MSIZE_1 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_EN;

    m_tim->ARR = timings.UnitsTotal;
    m_tim->CCR2 = timings.Unitsout0;
    m_tim->CCR4 = timings.Unitsout1;
    m_tim->CCR1 = timings.UnitsSample;
    m_tim->CR1 |= TIM_CR1_CEN;

    m_handlerTim.wait();

    __DSB();
    return (!!(m_in & data.inMask)) ^ data.inInvert;
  }    

  TimingGenerator(const TimingGenerator&) = delete;
  TimingGenerator& operator =(const TimingGenerator&) = delete;

//  friend class TimingGeneratorBus<TimingGenerator>;
};

extern "C" void maintask()
{
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Device %08lx%08lx%08lx is running at %lu heap size=%u\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq(), xPortGetFreeHeapSize());

  Tools::IdleMeasure::calibrate();

//  auto adc = Analog::CreateAdcStm32(0, 0, false);

  for (;;)
  {
    Tools::IdleMeasure im;

    OS::Thread::delay(1000);
    RT::stall(SystemCoreClock);

//    for(int i = 0; i < 10000; ++i)
//    {
//      adc->convert();
//    }

//    for (int i = 0; i < 1000; ++i)
//    {
//      receiver.receive();
//      OS::Thread::delay(1);
//    }

    unsigned tenths;
    printf("CPU IDLE=%02u.%01u%%\n", im.get(&tenths), tenths);
  }
}
