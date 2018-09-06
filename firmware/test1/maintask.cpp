#include <lib/common/utils.h>
#include <lib/common/math.h>
#include <lib/common/handlers.h>
#include <lib/common/stm32.h>
#include <lib/common/utils_stm32.h>
#include <lib/common/pin_stm32.h>
#include <lib/enc28j60/enc28j60_spi_stm32.h>
#include <lib/enc28j60/enc28j60_lwip.h>
#include <lib/analog/adc_stm32.h>
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

class RFControl
{
public:
  RFControl() = default;

  bool hasData() const
  {
    return m_data1Ready || m_data2Ready;
  }

  void getRaw(uint16_t** const timingsUs, size_t* const timings_size)
  {
    if (m_data1Ready)
    {
      *timingsUs = &m_timingsUs[0];
      *timings_size = m_dataEnd[0] + 1;
      m_data1Ready = false;
    }
    else if (m_data2Ready)
    {
      *timingsUs = &m_timingsUs[m_dataStart[1]];
      *timings_size = m_dataEnd[1] - m_dataStart[1] + 1;
      m_data2Ready = false;
    }
  }

  void continueReceiving()
  {
    if(m_state == State::STATUS_RECORDING_END)
    {
      m_state = State::STATUS_WAITING;
      m_data1Ready = false;
      m_data2Ready = false;
    }
  }

  void process(uint16_t const durationUs)
  {
    switch (m_state)
    {
    case State::STATUS_WAITING:
      if (probablyFooter(durationUs))
        startRecording(durationUs);
      break;
    case State::STATUS_RECORDING_0:
      recording(durationUs, 0);
      break;
    case State::STATUS_RECORDING_1:
      recording(durationUs, 1);
      verification(1);
      break;
    case State::STATUS_RECORDING_2:
      recording(durationUs, 2);
      verification(2);
      break;
    case State::STATUS_RECORDING_3:
      recording(durationUs, 3);
      verification(3);
      break;
    case State::STATUS_RECORDING_END:
      break;
    }
  }

protected:
  bool probablyFooter(uint16_t const durationUs) const
  {
    return durationUs >= c_minFooterUs; 
  }

  bool matchesFooter(uint16_t const durationUs) const
  {
    auto footerDeltaUs = m_footerUs/4;
    return m_footerUs - footerDeltaUs < durationUs && durationUs < m_footerUs + footerDeltaUs;
  }

  void startRecording(uint16_t const durationUs)
  {
    m_footerUs = durationUs;
    m_dataEnd[0] = 0;
    m_dataEnd[1] = 0;
    m_dataEnd[2] = 0;
    m_dataEnd[3] = 0;
    m_dataStart[0] = 0;
    m_dataStart[1] = 0;
    m_dataStart[2] = 0;
    m_dataStart[3] = 0;
    m_pack0EqualPack3 = true;
    m_pack1EqualPack3 = true;
    m_pack0EqualPack2 = true;
    m_pack1EqualPack2 = true;
    m_pack0EqualPack1 = true;
    m_data1Ready = false;
    m_data2Ready = false;
    m_state = State::STATUS_RECORDING_0;
  }

  void recording(uint16_t const durationUs, size_t const package)
  {
    if (matchesFooter(durationUs)) //test for footer (+-25%).
    {
      //Package is complete!!!!
      m_timingsUs[m_dataEnd[package]] = durationUs;
      m_dataStart[package + 1] = m_dataEnd[package] + 1;
      m_dataEnd[package + 1] = m_dataStart[package + 1];

      //Received more than 16 timings and start and end are the same footer then enter next state
      //less than 16 timings -> restart the package.
      if (m_dataEnd[package] - m_dataStart[package] >= 16)
      {
        if (m_state == State::STATUS_RECORDING_3)
          m_state = State::STATUS_RECORDING_END;
        else
          m_state = State(size_t(State::STATUS_RECORDING_0) + package + 1);
      }
      else
      {
        m_dataEnd[package] = m_dataStart[package];
        switch (package)
        {
          case 0:
            startRecording(durationUs); //restart
            break;
          case 1:
            m_pack0EqualPack1 = true;
            break;
          case 2:
            m_pack0EqualPack2 = true;
            m_pack1EqualPack2 = true;
            break;
          case 3:
            m_pack0EqualPack3 = true;
            m_pack1EqualPack3 = true;
            break;
        }
      }
    }
    else
    {
      //duration isnt a footer? this is the way.
      //if duration higher than the saved footer then the footer isnt a footer -> restart.
      if (durationUs > m_footerUs)
      {
        startRecording(durationUs);
      }
      //normal
      else if (m_dataEnd[package] < c_maxRecordings - 1)
      {
        m_timingsUs[m_dataEnd[package]] = durationUs;
        m_dataEnd[package]++;
      }
      //buffer reached end. Stop recording.
      else
      {
        m_state = State::STATUS_WAITING;
      }
    }
  }

  void verify(bool* const verifiyState, bool* const dataState, uint16_t const refValMax, uint16_t const refValMin, size_t const pos, size_t const package) const
  {
    if (*verifiyState && pos >= 0)
    {
      auto mainVal = m_timingsUs[pos];
      if (refValMin > mainVal || mainVal > refValMax)
      {
        //werte passen nicht
        *verifiyState = false;
      }
      if (m_state == State(size_t(State::STATUS_RECORDING_0) + package + 1) && *verifiyState == true)
      {
        *dataState = true;
      }
    }
  }

  void verification(size_t const package)
  {
    int refVal = m_timingsUs[m_dataEnd[package] - 1];
    int delta = refVal / 8 + refVal / 4; //+-37,5%
    int refValMin = refVal - delta;
    int refValMax = refVal + delta;
    int pos = m_dataEnd[package] - 1 - m_dataStart[package];

    switch (package)
    {
    case 1:
      verify(&m_pack0EqualPack1, &m_data1Ready, refValMax, refValMin, pos, package);
      break;
    case 2:
      verify(&m_pack0EqualPack2, &m_data1Ready, refValMax, refValMin, pos, package);
      verify(&m_pack1EqualPack2, &m_data2Ready, refValMax, refValMin, pos, package);
      if (m_state == State::STATUS_RECORDING_3 && m_data1Ready == false && m_data2Ready == false) {
        m_state = State::STATUS_WAITING;
      }
      break;
    case 3:
      if (!m_pack0EqualPack2)
        verify(&m_pack0EqualPack3, &m_data1Ready, refValMax, refValMin, pos, package);
      if (!m_pack1EqualPack2)
        verify(&m_pack1EqualPack3, &m_data2Ready, refValMax, refValMin, pos, package);
      if (m_state == State::STATUS_RECORDING_END && m_data1Ready == false && m_data2Ready == false) {
        m_state = State::STATUS_WAITING;
      }
      break;
    }
  }

protected:
  constexpr static size_t c_maxRecordings = 512;
  constexpr static uint16_t c_minFooterUs = 1500;      // fixme 3500;

  uint16_t m_timingsUs[c_maxRecordings];
  bool m_data1Ready = false;
  bool m_data2Ready = false;
  size_t m_dataStart[5] = {};
  size_t m_dataEnd[5] = {};
  enum class State {
    STATUS_WAITING,
    STATUS_RECORDING_0,
    STATUS_RECORDING_1,
    STATUS_RECORDING_2,
    STATUS_RECORDING_3,
    STATUS_RECORDING_END,
  } m_state = State::STATUS_WAITING;
  uint16_t m_footerUs = 0;
  bool m_pack0EqualPack1 = false;
  bool m_pack0EqualPack2 = false;
  bool m_pack0EqualPack3 = false;
  bool m_pack1EqualPack2 = false;
  bool m_pack1EqualPack3 = false;
};

RFControl s_ctl;
uint16_t s_durationUs = 0;

class Receiver
{
public:
  Receiver()
    : m_timer(RT::CreateHiresTimer(TIM7, RT::HiresTimer::Callback::make<Receiver, &Receiver::periodic>(*this)))
  {
    m_timer->start(1000000 / c_samplePeriodUs);
  }

  void test()
  {
    uint16_t value;
    while (m_samples.load(value))
    {
//      printf("%u\n", value);

      if (unsigned(s_durationUs) + unsigned(value) < std::numeric_limits<uint16_t>::max())
        s_durationUs += value;
      else
        s_durationUs = std::numeric_limits<uint16_t>::max();

      if (value == c_idlePeriodUs)
        continue;

//      printf("%hu\n", s_durationUs);
      s_ctl.process(s_durationUs);
      s_durationUs = 0;
    }

//    printf("\n");

    if(s_ctl.hasData()) {
      uint16_t *timingsUs;
      size_t timings_size;
      s_ctl.getRaw(&timingsUs, &timings_size);
      printf("Code found:\n");
      for(size_t i=0; i < timings_size; ++i) {
        auto timingUs = timingsUs[i];
//        Serial.print(timingUs);
//        Serial.write(' ');
        printf("%hu ", timingUs);
        if((i+1)%16 == 0) {
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
    auto currentDurationUs = m_currentDurationUs;
    
    if (m_filter.next(m_in.read()))
    {
      m_samples.store(currentDurationUs);
      currentDurationUs = c_samplePeriodUs;
    }
    else
    {
      currentDurationUs += c_samplePeriodUs;
      if (currentDurationUs >= c_idlePeriodUs)
      {
        m_samples.store(currentDurationUs);
        currentDurationUs = 0;
      }
    }

    m_currentDurationUs = currentDurationUs;
  }

protected:
  constexpr static uint16_t c_idlePeriodUs = 3500;

  constexpr static uint16_t c_samplePeriodUs = 10;
  constexpr static size_t c_samples = 200;

  constexpr static int c_filterFrame = 10;
  constexpr static int c_filterLower = 3;
  constexpr static int c_filterUpper = 8;

  Pin::In m_in = Pin::Def(GPIOE, GPIO_PIN_1, false);

  std::unique_ptr<RT::HiresTimer> m_timer;
  math::BounceFilter<uint32_t, c_filterFrame, c_filterLower, c_filterUpper> m_filter;
  uint16_t m_currentDurationUs = 0;
  mstd::NonlockedFifo<uint16_t, c_samples> m_samples;
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

    unsigned tenths;
    printf("CPU IDLE=%02u.%01u%%\n", im.get(&tenths), tenths);
  }
}
