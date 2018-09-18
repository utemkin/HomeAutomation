#include <lib/common/utils.h>
#include <lib/common/math.h>
#include <lib/common/handlers.h>
#include <lib/common/stm32.h>
#include <lib/common/utils_stm32.h>
#include <lib/common/pin_stm32.h>
#include <lib/enc28j60/enc28j60_spi_stm32.h>
#include <lib/enc28j60/enc28j60_lwip.h>
#include <lib/analog/adc_stm32.h>
#include <lib/remotecontrol/RFControl.h>
#include <frozen/frozen.h>
#include "main.h"
#include "adc.h"

template<typename T, T initialOffset, int shift>
class DcBlocker
{
public:
  T next(T const x)
  {
    T y = x - mstd::rsar<shift>(m_scaledOffset);
    m_scaledOffset += y;
    return y;
  }

protected:
  T m_scaledOffset = initialOffset << shift;
};

template<typename T, int shift>
class FpScaler
{
public:
  FpScaler(T scale)
    : m_scale(scale)
  {
  }

  constexpr T apply(T const v) const
  {
    return mstd::rsar<shift>(v * m_scale);
  }

protected:
  T const m_scale;
};

template<class T>
class Registry : mstd::noncopyable
{
public:
  template<class Tc, typename... Args>
  Tc* make(Args ...args)
  {
    auto proc = new Tc(args...);
    proc->m_next = nullptr;
    *m_lastPtr = proc;
    m_lastPtr = &proc->m_next;
    return proc;
  }

  T* get(unsigned n) const
  {
    for (auto proc = m_first; proc; proc = proc->m_next, --n)
      if (n == 0)
        return proc;

    return nullptr;
  }

  template<typename... Args>
  void foreach(void(T::*func)(Args...), Args ...args) const
  {
    for (auto proc = m_first; proc; proc = proc->m_next)
      (proc->*func)(args...);
  }

protected:
  T* m_first = nullptr;
  T** m_lastPtr = &m_first;
};

class VMeter : mstd::noncopyable
{
public:
  using DataType = int32_t;
  using Blocker = DcBlocker<DataType, 1 << 11, 8>;
  using Scaler = FpScaler<DataType, 16>;

public:
  VMeter(const volatile uint16_t* const ch, Scaler scaler)
    : m_ch(ch)
    , m_scaler(scaler)
  {
  }

  void process()
  {
    auto const v = m_scaler.apply(m_blocker.next(*m_ch));
    m_v_VDIV10 = v;
    m_sumSq_v_VDIV10 += v * v;
    ++m_count;
  }

protected:
  VMeter* m_next;
  const volatile uint16_t* const m_ch;
  Scaler const m_scaler;
  Blocker m_blocker;
  DataType m_v_VDIV10;
  DataType m_sumSq_v_VDIV10 = 0;
  unsigned m_count = 0;

friend class Registry<VMeter>;
};

class AMeter : mstd::noncopyable
{
public:
  using DataType = int32_t;
  using Blocker = DcBlocker<DataType, 1 << 11, 8>;
  using Scaler = FpScaler<DataType, 16>;

public:
  AMeter(const volatile uint16_t* const ch, Scaler scaler, VMeter* const vmeter)
    : m_ch(ch)
    , m_scaler(scaler)
    , m_vmeter(vmeter)
  {
  }

  void process()
  {
    auto const a = m_scaler.apply(m_blocker.next(*m_ch));
    m_a_mA = a;
    m_sumSq_a_mA += a * a;
//    m_sum_a_mA_mul_v_VDIV10 += a * m_vmeter->m_v_VDIV10;
    ++m_count;
  }

protected:
  AMeter* m_next;
  const volatile uint16_t* const m_ch;
  Scaler const m_scaler;
  VMeter* const m_vmeter;
  Blocker m_blocker;
  DataType m_a_mA;
  DataType m_sumSq_a_mA = 0;
  DataType m_sum_a_mA_mul_v_VDIV10 = 0;
  unsigned m_count = 0;

friend class Registry<AMeter>;
};

using VMeters = Registry<VMeter>;
using AMeters = Registry<AMeter>;

struct Meters
{
  VMeters vMeters;
  AMeters aMeters;

  void process()
  {
    vMeters.foreach(&VMeter::process);
    aMeters.foreach(&AMeter::process);
  }
};

class Sampler
{
public:
  Sampler()
    : m_timer(RT::CreateHiresTimer(TIM7, RT::HiresTimer::Callback::make<Sampler, &Sampler::periodic>(*this)))
    , m_adc(Analog::CreateAdcStm32(Pin::Def(ADC_SELECT2_GPIO_Port, ADC_SELECT2_Pin, false), Analog::Adc::Callback::make<Sampler, &Sampler::adcReady>(*this)))
  {
    for (size_t i = 0;; ++i)
    {
      auto const ch = m_adc->channel(i);
      if (!ch)
        break;
      if (i == 0)
        m_meters.vMeters.make<VMeter>(ch, VMeter::Scaler(100));
      else
        m_meters.aMeters.make<AMeter>(ch, AMeter::Scaler(100), m_meters.vMeters.get(0));
    }
    
    m_timer->start(5000);
  }

protected:
  void periodic()
  {
    m_adc->start();
  }

  void adcReady()
  {
    m_meters.process();
  }

protected:
  std::unique_ptr<RT::HiresTimer> m_timer;
  std::unique_ptr<Analog::Adc> m_adc;
  Meters m_meters;
};

class Decoder
{
public:
  using DurationUs = uint16_t;
  struct Cycle
  {
    DurationUs oneDurationUs;
    DurationUs zeroDurationUs;
  };

public:
  void process(bool const bit, DurationUs const durationUs)
  {
    if (bit == m_lastBit)
    {
      auto& d = bit ? m_lastCycle.oneDurationUs : m_lastCycle.zeroDurationUs;
      d = mstd::badd(d, durationUs);
    }
    else
    {
      if (bit)
      {
        process();
        m_lastCycle.oneDurationUs = durationUs;
        m_lastCycle.zeroDurationUs = 0;
      }
      else
      {
        m_lastCycle.zeroDurationUs = durationUs;
      }
      m_lastBit = bit;
    }
  }

protected:
  void process()
  {
    printf("%hu %hu\n", m_lastCycle.oneDurationUs, m_lastCycle.zeroDurationUs);

    switch(m_phase)
    {
    case 0:
      if (mstd::badd(m_lastCycle.oneDurationUs, m_lastCycle.zeroDurationUs) >= c_syncThrethold)
      {
        m_phase = 1;
        break;
      }
    case 1:
      if (mstd::badd(m_lastCycle.oneDurationUs, m_lastCycle.zeroDurationUs) >= c_syncThrethold)
        break;

      m_dataSize = 0;
      m_phase = 2;
    case 2:
      break;
    }
  }

protected:
  constexpr static DurationUs c_syncThrethold = 3500;
  constexpr static size_t c_maxData = 132;

  bool m_lastBit = true;
  Cycle m_lastCycle = {100, 0};
  int m_phase = 0;
  std::array<Cycle, c_maxData> m_data;
  size_t m_dataSize;
};

RC::RFControl s_ctl;
RC::RFControl::DurationUs s_durationUs = 0;

Decoder s_decoder;

class Receiver
{
public:
  using DurationUs = int16_t;

public:
  Receiver()
    : m_timer(RT::CreateHiresTimer(TIM7, RT::HiresTimer::Callback::make<Receiver, &Receiver::periodic>(*this)))
  {
    m_timer->start(1000000 / c_samplePeriodUs);
  }

  void test()
  {
    DurationUs durationUs;
    while (m_samples.load(durationUs))
    {
//      printf("%hi\n", durationUs);
      s_decoder.process(durationUs < 0 ? false : true, durationUs < 0 ? -durationUs : durationUs);

#if 0
      if (durationUs < 0)
        durationUs = -durationUs;

      if (s_durationUs < std::numeric_limits<DurationUs>::max() - durationUs)
        s_durationUs += durationUs;
      else
        s_durationUs = std::numeric_limits<DurationUs>::max();

      if (durationUs == c_maxPeriodUs)
        continue;

//      printf("%hi\n", s_durationUs);

      s_ctl.process(s_durationUs);
      s_durationUs = 0;
#endif
    }

//    printf("\n");

    if(s_ctl.hasData()) {
      const RC::RFControl::DurationUs* timingsUs;
      size_t timings_size;
      s_ctl.getRaw(&timingsUs, &timings_size);
      printf("Code found:\n");
      for(size_t i = 0; i < timings_size; ++i) {
        auto timingUs = timingsUs[i];
//        Serial.print(timingUs);
//        Serial.write(' ');
        printf("%hu ", timingUs);
        if((i + 1) % 16 == 0) {
//          Serial.write('\n');
          printf("\n");
        }
      }
//      Serial.write('\n');
//      Serial.write('\n');
      printf("\n\n");
      s_ctl.continueReceiving();
    }
  }

protected:
  void periodic()
  {
//    putchar('0' + m_in.read());

    auto const lastState = m_filter.getState();
    auto currentDurationUs = m_currentDurationUs + c_samplePeriodUs;
    
    if (m_filter.next(m_in.read()) || currentDurationUs >= c_maxPeriodUs)
    {
      m_samples.store(lastState ? currentDurationUs : -currentDurationUs);
      currentDurationUs = 0;
    }

    m_currentDurationUs = currentDurationUs;
  }

protected:
  constexpr static DurationUs c_samplePeriodUs = 10;
  constexpr static size_t c_samples = 200;

  constexpr static DurationUs c_maxPeriodUs = 10000 - c_samplePeriodUs;

  //this filters out all pulses shorter than 9 samples
  constexpr static int c_filterShift = 3;
  constexpr static unsigned c_filterLowerPercent = 25;
  constexpr static unsigned c_filterUpperPercent = 75;

  Pin::In m_in = Pin::Def(GPIOE, GPIO_PIN_1, false);

  std::unique_ptr<RT::HiresTimer> m_timer;
  math::BounceFilter<c_filterShift, c_filterLowerPercent, c_filterUpperPercent> m_filter;
  DurationUs m_currentDurationUs = 0;
  mstd::NonlockedFifo<DurationUs, c_samples> m_samples;
};

extern "C" void maintask()
{
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Device %08lx%08lx%08lx is running at %lu heap size=%u\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq(), xPortGetFreeHeapSize());

  Tools::IdleMeasure::calibrate();

//  Pin::Def pd;
//  pd.load("PC1");

//  Enc28j60::LwipNetif::initLwip();
//  auto netif = Enc28j60::CreateLwipNetif(Enc28j60::CreateSpiStm32(SPI1, Pin::Def(SPI1_CS_GPIO_Port, SPI1_CS_Pin, true)));
//  netif->setDefault();
//  netif->startDhcp();

//  auto adc = Analog::CreateAdcStm32(SWITCH_ADC_GPIO_Port, SWITCH_ADC_Pin, false);
//  auto adc = Analog::CreateAdcStm32(0, 0, false);

//  uint32_t clk = DWT->CYCCNT;
//  adc->convert();
////  OS::Thread::yield();
////  portYIELD();
//  printf("%lu\n", DWT->CYCCNT - clk);

//  C c;
//  auto ht = RT::CreateHiresTimer(TIM7, RT::HiresTimer::Callback::make<C, &C::timer>(c));
//  auto ht = RT::CreateHiresTimer(TIM7, RT::HiresTimer::Callback::make<&C::timer2>());
//  ht->start(72000000 / 1000);
//  OS::Thread::delay(1000);

//  Sampler sampler;

  Receiver receiver;

  for(;;)
  {
    Tools::IdleMeasure im;

//    OS::Thread::delay(1000);

//    for(int i = 0; i < 10000; ++i)
//    {
//      adc->convert();
//    }

    for (int i = 0; i < 100; ++i)
    {
      receiver.test();
      OS::Thread::delay(10);
    }

//    unsigned tenths;
//    printf("CPU IDLE=%02u.%01u%%\n", im.get(&tenths), tenths);
  }
}
