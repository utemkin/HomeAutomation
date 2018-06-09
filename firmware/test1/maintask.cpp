#include <common/utils.h>
#include <common/handlers.h>
#include <common/stm32.h>
#include <common/utils_stm32.h>
#include <enc28j60/enc28j60spistm32.h>
#include <enc28j60/enc28j60lwip.h>
#include <analog/adcstm32.h>
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
    : m_timer(RT::CreateHiresTimer(TIM7, RT::HiresTimer::Callback::make<Sampler, &Sampler::timer>(*this)))
    , m_adc(Analog::CreateAdcStm32(SWITCH_ADC_GPIO_Port, SWITCH_ADC_Pin, false, Analog::Adc::Callback::make<Sampler, &Sampler::adcReady>(*this)))
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
  void timer()
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

extern "C" void maintask()
{
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Device %08lx%08lx%08lx is running at %lu heap size=%u\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq(), xPortGetFreeHeapSize());

  Tools::IdleMeasure::calibrate();

//  Enc28j60::LwipNetif::initLwip();
//  auto netif = Enc28j60::CreateLwipNetif(Enc28j60::CreateSpiStm32(SPI1, SPI1_CS_GPIO_Port, SPI1_CS_Pin, true));
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

  Sampler sampler;

  for(;;)
  {
    Tools::IdleMeasure im;

    OS::Thread::delay(1000);

//    for(int i = 0; i < 10000; ++i)
//    {
//      adc->convert();
//    }

    unsigned tenths;
    printf("CPU IDLE=%02u.%01u%%\n", im.get(&tenths), tenths);
  }
}
