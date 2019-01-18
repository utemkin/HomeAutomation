#include <lib/common/utils.h>
#include <lib/common/handlers.h>
#include <lib/analog/adc_stm32.h>
#include <lib/microlan/microlan.h>
#include <lib/common/hal.h>
#include <lib/common/pin_stm32.h>
#include <limits>

namespace MicroLan
{
  class TimingGenerator
  {
  public:
    struct Timings
    {
      uint16_t ticksTotal;
      uint16_t ticksOut0;
      uint16_t ticksOut1;
      uint16_t ticksSample;
      Timings(TimingGenerator& timingGenerator, unsigned const ticksTotalNs, unsigned const ticksOut0Ns, unsigned const ticksOut1Ns, unsigned const ticksSampleNs)
        : ticksTotal(timingGenerator.toTicks(ticksTotalNs))
        , ticksOut0(timingGenerator.toTicks(ticksOut0Ns))
        , ticksOut1(timingGenerator.toTicks(ticksOut1Ns))
        , ticksSample(timingGenerator.toTicks(ticksSampleNs))
      {
      }
    };

    struct Data
    {
      uint32_t outAddr;
      uint32_t out1;
      uint32_t out0;
      uint32_t inAddr;
      uint16_t inMask;
      bool inInvert;
      Data(const Pin::Def& out, const Pin::Def& in)
        : outAddr(uint32_t(&out.gpio()->BSRR))
        , out1(out.invert() ? uint32_t(out.pin()) << 16 : out.pin())
        , out0(out.invert() ? out.pin() : uint32_t(out.pin()) << 16)
        , inAddr(uint32_t(&in.gpio()->IDR))
        , inMask(in.pin())
        , inInvert(in.invert())
      {
      }
    };

  public:
    //OUT/IN
    //F1:
    //  TIM2 - DMA1C7/DMA1C5
    //F4:
    //  TIM1 - DMA2S6C0/DMA2S1C6 or DMA2S6C0/DMA2S2C6 or DMA2S6C0/DMA2S3C6 or DMA2S6C0/DMA2S4C6
    //  TIM8 - DMA2S2C0/DMA2S3C7 or DMA2S2C0/DMA2S4C7 or DMA2S2C0/DMA2S7C7
    TimingGenerator()
      : m_handlerTim(Irq::Handler::Callback::make<TimingGenerator, &TimingGenerator::handleTimIrq>(*this))
  #if defined(STM32F1)
      , m_tim(TIM2)                   //fixme
      , m_tim_CC_Out0(&m_tim->CCR2)   //fixme
      , m_tim_CC_Out1(&m_tim->CCR4)   //fixme
      , m_tim_CC_Sample(&m_tim->CCR1) //fixme
      , m_IRQn(TIM2_IRQn)             //fixme
      , m_dmaOut(DMA1_Stream6, 3, HAL::DMALine::c_config_PRIO_LOW | HAL::DMALine::c_config_M32 | HAL::DMALine::c_config_P32 | HAL::DMALine::c_config_MINC | HAL::DMALine::c_config_M2P, 0, 0)   //fixme
      , m_dmaIn(DMA1_Stream5, 3, HAL::DMALine::c_config_PRIO_LOW | HAL::DMALine::c_config_M32 | HAL::DMALine::c_config_P32 | HAL::DMALine::c_config_MINC | HAL::DMALine::c_config_P2M, 0, 0)    //fixme
  #elif defined(STM32F4)
      , m_tim(TIM1)                   //fixme
      , m_tim_CC_Out0(&m_tim->CCR2)   //fixme
      , m_tim_CC_Out1(&m_tim->CCR3)   //fixme
      , m_tim_CC_Sample(&m_tim->CCR1) //fixme
      , m_IRQn(TIM1_UP_TIM10_IRQn)    //fixme
      , m_dmaOut(DMA2_Stream6, 0, HAL::DMALine::c_config_PRIO_LOW | HAL::DMALine::c_config_M32 | HAL::DMALine::c_config_P32 | HAL::DMALine::c_config_MINC | HAL::DMALine::c_config_M2P, 0, 0)   //fixme
      , m_dmaIn(DMA2_Stream1, 6, HAL::DMALine::c_config_PRIO_LOW | HAL::DMALine::c_config_M32 | HAL::DMALine::c_config_P32 | HAL::DMALine::c_config_MINC | HAL::DMALine::c_config_P2M, 0, 0)    //fixme
  #else
  #error Unsupported architecture
  #endif
    {
      init();
      start();
    }

    void lock()
    {
      m_mutex.lock();
    }
    void unlock()
    {
      m_mutex.unlock();
    }

    bool touch(const Timings& timings, const Data& data)
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
    OS::Mutex m_mutex;
    Irq::SemaphoreHandler m_handlerTim;
    TIM_TypeDef* const m_tim;
    __IO uint32_t* const m_tim_CC_Out0;
    __IO uint32_t* const m_tim_CC_Out1;
    __IO uint32_t* const m_tim_CC_Sample;
    IRQn_Type const m_IRQn;
    HAL::DMALine m_dmaOut;
    HAL::DMALine m_dmaIn;
    uint16_t m_prescale;
    uint32_t m_unitClock;
    volatile uint32_t m_in;
    volatile uint32_t m_out[2];

  protected:

    // Must do permanent initializations like turning on peripheral clocks, setting interrupt priorities, installing interrupt handlers
    void init()
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

    // Must put peripheral to normal operational state either after init() or stop()
    void start()
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

    // Must stop all peripheral activities (even if called the middle of operation) and prevent further interrupt|DMA requests
    void stop()
    {
      HAL_NVIC_DisableIRQ(m_IRQn);
      m_dmaIn.stop();
      m_dmaOut.stop();
      m_tim->CR1 = 0;
    }

    uint16_t toTicks(unsigned const intervalNs)
    {
      return Bus::toUnit(intervalNs, m_unitClock) + 1;
    }

    TimingGenerator(const TimingGenerator&) = delete;
    TimingGenerator& operator =(const TimingGenerator&) = delete;
  };

  class TimingGeneratorBus : public Bus
  {
  public:
    TimingGeneratorBus(std::shared_ptr<TimingGenerator> const timingGenerator, const Pin::Def& out, const Pin::Def& in)
      : m_timingGenerator(timingGenerator)
      , m_resetTimings(*m_timingGenerator.get(), c_G + c_H + c_I + c_J, c_G, c_G + c_H, c_G + c_H + c_I)
      , m_readTimings(*m_timingGenerator.get(), c_A + c_E + c_F, 0, c_A, c_A + c_E)
      , m_writeZeroTimings(*m_timingGenerator.get(), c_C + c_D, 0, c_C, c_A + c_E)
      , m_writeOneTimings(*m_timingGenerator.get(), c_A + c_B, 0, c_A, c_A + c_E)
      , m_data(out, in)
    {
      Pin::Out(out).toActive();
    }

    virtual Capabilities capabilities() const override
    {
      return {
        overdriveSupported : false,
        strengthMicroampsStrongPoolup5V : 0,
        strengthMicroampsPulse12V : 0,
        strengthMicroampsExternal5V : 15000,
      };
    }

  protected:
    virtual Status lock(const Options& options) override
    {
      auto status = Bus::lock(options);
      if (status != Status::Success)
        return status;

      m_timingGenerator->lock();
      return Status::Success;
    }

    virtual void unlock()
    {
      m_timingGenerator->unlock();
      Bus::unlock();
    }

    virtual Status reset(bool& presence) override
    {
      presence = !m_timingGenerator->touch(m_resetTimings, m_data);
      return Status::Success;
    }

    virtual Status read(bool& bit, bool /*last*/) override
    {
      bit = m_timingGenerator->touch(m_readTimings, m_data);
      return Status::Success;
    }

    virtual Status write(bool bit, bool /*last*/) override
    {
      if (m_timingGenerator->touch(bit ? m_writeOneTimings : m_writeZeroTimings, m_data) != bit)
        return bit ? Status::BusShortToGnd : Status::BusShortToVdd;

      return Status::Success;
    }

  protected:
    std::shared_ptr<TimingGenerator> const m_timingGenerator;
    TimingGenerator::Timings const m_resetTimings;
    TimingGenerator::Timings const m_readTimings;
    TimingGenerator::Timings const m_writeZeroTimings;
    TimingGenerator::Timings const m_writeOneTimings;
    TimingGenerator::Data const m_data;
  };
}

extern "C" void maintask()
{
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Device %08lx%08lx%08lx is running at %lu heap size=%u\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq(), xPortGetFreeHeapSize());

  Tools::IdleMeasure::calibrate();

//  auto adc = Analog::CreateAdcStm32(0, 0, false);

  auto gen = std::make_shared<MicroLan::TimingGenerator>();
  MicroLan::TimingGeneratorBus bus(gen, Pin::Def(GPIOE, GPIO_PIN_0, false), Pin::Def(GPIOE, GPIO_PIN_1, false));

  {
  /*
found 28:0000037ee845
found 28:0000037efbfd
  */
    MicroLan::Status status;
    MicroLan::Enumerator enumerator(bus);
    MicroLan::RomCode romCode;

    while ((status = enumerator.next(romCode)) == MicroLan::Status::Success)
    {
      printf("found %02x:%04x%08x\n", romCode.family(), unsigned(romCode.serialNumber() >> 32), unsigned(romCode.serialNumber() & 0xffffffff));
    }
    printf("enum status %u\n", unsigned(status));
  }

  int prevTemp = 0;
  for (int i = 0; ; ++i)
  {
    MicroLan::Status status;
    MicroLan::Status status2;
    MicroLan::RomCode romCode(0x28, 0x0000037ee845);
    MicroLan::DS18B20::Device device(bus, romCode);

    status = MicroLan::Device::executeWithMatchRom(device, {overdrive : false, powerMode : MicroLan::PowerMode::External5V}, &MicroLan::DS18B20::Device::convertT, 1);

    MicroLan::DS18B20::Scratchpad sp;
    status2 = MicroLan::Device::executeWithMatchRom(device, {}, &MicroLan::DS18B20::Device::readScratchpad, std::ref(sp));
    int temp = sp.temp();

    static const char* frac[] = {
      "0000", "0625", "1250", "1875",
      "2500", "3125", "3750", "4375",
      "5000", "5625", "6250", "6875",
      "7500", "8125", "8750", "9375"
    };
    int dif = temp - prevTemp;
    printf("%i %li convertT status %u readScratchpad status %u CRC %s temp %i.%s %c%i.%s\n",
      i, osKernelSysTick(), unsigned(status), unsigned(status2), sp.Crc == sp.calcCrc() ? "ok" : "fail",
      temp >> 4, frac[temp & 0xf], dif == 0 ? ' ' : dif > 0 ? '+' : '-', abs(dif) >> 4, frac[abs(dif) & 0xf]);
    prevTemp = temp;
  }

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
