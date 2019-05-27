#pragma once

#include <lib/microlan/microlan.h>
#include <lib/common/hal.h>
#include <lib/common/handlers.h>

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
    //  TIM2 - DMA1C7/DMA1C1 or DMA1C7/DMA1C5
    //F4:
    //  TIM1 - DMA2S6C0/DMA2S1C6 or DMA2S6C0/DMA2S2C6 or DMA2S6C0/DMA2S3C6 or DMA2S6C0/DMA2S4C6
    //  TIM8 - DMA2S2C0/DMA2S3C7 or DMA2S2C0/DMA2S4C7 or DMA2S2C0/DMA2S7C7
    TimingGenerator();

    void lock()
    {
      m_mutex.lock();
    }
    void unlock()
    {
      m_mutex.unlock();
    }

    bool touch(const Timings& timings, const Data& data);

  protected:
    bool handleTimIrq(Hal::Irq irq);

  protected:
    Os::Mutex m_mutex;
    Irq::SemaphoreHandler m_handlerTim;
    TIM_TypeDef* const m_tim;
    __IO uint32_t* const m_tim_CC_Out0;
    __IO uint32_t* const m_tim_CC_Out1;
    __IO uint32_t* const m_tim_CC_Sample;
    Hal::Irq const m_irq;
    std::unique_ptr<Hal::DmaLine> m_dmaOut;
    std::unique_ptr<Hal::DmaLine> m_dmaIn;
    uint16_t m_prescale;
    uint32_t m_unitClock;
    volatile uint32_t m_in;
    volatile uint32_t m_out[2];

  protected:

    // Must do permanent initializations like turning on peripheral clocks, setting interrupt priorities, installing interrupt handlers
    void init();

    // Must put peripheral to normal operational state either after init() or stop()
    void start();

    // Must stop all peripheral activities (even if called the middle of operation) and prevent further interrupt|DMA requests
    void stop();

    uint16_t toTicks(unsigned const intervalNs) const;

    TimingGenerator(const TimingGenerator&) = delete;
    TimingGenerator& operator =(const TimingGenerator&) = delete;
  };
}
