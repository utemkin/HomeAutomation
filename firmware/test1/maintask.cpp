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
#include <lib/remotecontrol/decoder.h>
#include <lib/remotecontrol/receiver.h>
#include <lib/remotecontrol/receiver_stm32.h>
#include <lib/microlan/microlan_stm32.h>
#include <frozen/frozen.h>
#include "main.h"

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
    : m_timer(Rt::CreateHiresTimer(TIM7, Rt::HiresTimer::Callback::make<Sampler, &Sampler::periodic>(*this)))
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

  void adcReady(bool)
  {
    m_meters.process();
  }

protected:
  std::unique_ptr<Rt::HiresTimer> m_timer;
  std::unique_ptr<Analog::Adc> m_adc;
  Meters m_meters;
};

class MyReceiver : mstd::noncopyable
{
public:
  MyReceiver(TIM_TypeDef* tim, const Pin::Def& pin)
    : m_receiver(RC::CreateReceiverStm32(tim, pin))
  {
  }

  void receive()
  {
    RC::Receiver::DurationUs durationUs;
    while (m_receiver->receive(durationUs))
    {

//      printf("%hi\n", durationUs);

      RC::Decoder::Message message;
      m_decoder->process(durationUs > 0, durationUs > 0 ? durationUs : -durationUs, message);
    }
  }

protected:
  std::unique_ptr<RC::Receiver> m_receiver;
  std::unique_ptr<RC::Decoder> m_decoder = std::make_unique<RC::Decoder>();
};

class RFControlReceiver : mstd::noncopyable
{
public:
  RFControlReceiver(TIM_TypeDef* tim, const Pin::Def& pin)
    : m_receiver(RC::CreateReceiverStm32(tim, pin))
  {
  }

  void receive()
  {
    RC::Receiver::DurationUs durationUs;
    while (m_receiver->receive(durationUs))
    {

//      printf("%hi\n", durationUs);

      auto const bit = durationUs > 0;
      RC::RFControl::DurationUs const d = durationUs > 0 ? durationUs : -durationUs;
      if (bit == m_lastBit)
      {
        m_durationUs = mstd::badd(m_durationUs, d);
        continue;
      }

      m_lastBit = bit;

//      printf("%hi\n", s_durationUs);

      m_decoder->process(m_durationUs);
      m_durationUs = d;

      if(m_decoder->hasData()) {
        const RC::RFControl::DurationUs* timingsUs;
        size_t timings_size;
        m_decoder->getRaw(&timingsUs, &timings_size);
        printf("Code found:\n");
        for(size_t i = 0; i < timings_size; ++i) {
          auto timingUs = timingsUs[i];
  //        Serial.print(timingUs);
  //        Serial.write(' ');
          printf("%hu ", timingUs);
          if ((i + 1) % 16 == 0) {
  //          Serial.write('\n');
            printf("\n");
          }
        }
  //      Serial.write('\n');
  //      Serial.write('\n');
        printf("\n\n");
        m_decoder->continueReceiving();
      }
    }
  }

protected:
  std::unique_ptr<RC::Receiver> m_receiver;
  std::unique_ptr<RC::RFControl> m_decoder = std::make_unique<RC::RFControl>();
  bool m_lastBit = true;
  RC::RFControl::DurationUs m_durationUs = 0;
};

extern "C" void maintask()
{
  uint32_t uid[] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
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

//  MyReceiver receiver(TIM7, Pin::Def(GPIOE, GPIO_PIN_1, false));
  RFControlReceiver receiver(TIM7, Pin::Def(GPIOE, GPIO_PIN_1, false));

  auto gen = std::make_shared<MicroLan::TimingGenerator>();
  MicroLan::TimingGeneratorBus<MicroLan::TimingGenerator> bus(gen, Pin::Def(GPIOE, GPIO_PIN_0, false), Pin::Def(GPIOE, GPIO_PIN_1, false));

  for (;;)
  {
    Tools::IdleMeasure im;

//    OS::Thread::delay(1000);

//    for(int i = 0; i < 10000; ++i)
//    {
//      adc->convert();
//    }

    for (int i = 0; i < 1000; ++i)
    {
      receiver.receive();
      Os::Thread::delay(1);
    }

//    unsigned tenths;
//    printf("CPU IDLE=%02u.%01u%%\n", im.get(&tenths), tenths);
  }

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

    Os::Thread::delay(1000);
    Rt::stall(SystemCoreClock);

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
