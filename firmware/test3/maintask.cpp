#include <lib/common/utils.h>
#include <lib/common/utils_stm32.h>
#include <lib/enc28j60/enc28j60_spi_stm32.h>
#include <lib/analog/adc_stm32.h>
#include <lib/microlan/microlan_stm32.h>
#include <lib/analog/adc_stm32.h>
#include <lib/common/hal.h>
#include <limits>

namespace Hal
{
  class System : mstd::noncopyable
  {
  public:

  protected:
    System() = default;
    static System& instance()
    {
      static System instance{};
      return instance;
    }

  protected:
    Os::Mutex m_mutex;
  };
}

class Sampler
{
public:
  Sampler()
    : m_timer(Rt::CreateHiresTimer(TIM7, Rt::HiresTimer::Callback::make<Sampler, &Sampler::periodic>(*this)))
    , m_adc(Analog::CreateAdcStm32(Analog::Adc::Callback::make<Sampler, &Sampler::adcReady>(*this)))
  {
//    for (size_t i = 0;; ++i)
//    {
//      auto const ch = m_adc->channel(i);
//      if (!ch)
//        break;
//      if (i == 0)
//        m_meters.vMeters.make<VMeter>(ch, VMeter::Scaler(100));
//      else
//        m_meters.aMeters.make<AMeter>(ch, AMeter::Scaler(100), m_meters.vMeters.get(0));
//    }
    
    __HAL_DBGMCU_FREEZE_TIM7();
    m_timer->start(5000);
  }

protected:
  void periodic()
  {
    m_adc->start();
  }

  void adcReady(bool)
  {
//    m_meters.process();
  }

protected:
  std::unique_ptr<Rt::HiresTimer> m_timer;
  std::unique_ptr<Analog::Adc> m_adc;
//  Meters m_meters;
};

extern "C" void maintask()
{
  uint32_t uid[] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
  printf("Device %08lx%08lx%08lx is running at %lu heap size=%u\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq(), xPortGetFreeHeapSize());

  Tools::IdleMeasure::calibrate();

  Os::Queue<int, 1> q = {};
  q.push(1);
  q.push(1, 1000);
  int r;
  q.pop(r);
  q.pop(r, 0);


//  auto gen = std::make_shared<MicroLan::TimingGenerator>();
//  MicroLan::TimingGeneratorBus<MicroLan::TimingGenerator> bus(gen, Pin::Def(GPIOE, GPIO_PIN_0, false), Pin::Def(GPIOE, GPIO_PIN_1, false));
//
//  {
//  /*
//found 28:0000037ee845
//found 28:0000037efbfd
//  */
//    MicroLan::Status status;
//    MicroLan::Enumerator enumerator(bus);
//    MicroLan::RomCode romCode;
//
//    while ((status = enumerator.next(romCode)) == MicroLan::Status::Success)
//    {
//      printf("found %02x:%04x%08x\n", romCode.family(), unsigned(romCode.serialNumber() >> 32), unsigned(romCode.serialNumber() & 0xffffffff));
//    }
//    printf("enum status %u\n", unsigned(status));
//  }
//
//  int prevTemp = 0;
//  for (int i = 0; ; ++i)
//  {
//    MicroLan::Status status;
//    MicroLan::Status status2;
//    MicroLan::RomCode romCode(0x28, 0x0000037ee845);
//    MicroLan::DS18B20::Device device(bus, romCode);
//
//    status = MicroLan::Device::executeWithMatchRom(device, {overdrive : false, powerMode : MicroLan::PowerMode::External5V}, &MicroLan::DS18B20::Device::convertT, 1);
//
//    MicroLan::DS18B20::Scratchpad sp;
//    status2 = MicroLan::Device::executeWithMatchRom(device, {}, &MicroLan::DS18B20::Device::readScratchpad, std::ref(sp));
//    int temp = sp.temp();
//
//    static const char* frac[] = {
//      "0000", "0625", "1250", "1875",
//      "2500", "3125", "3750", "4375",
//      "5000", "5625", "6250", "6875",
//      "7500", "8125", "8750", "9375"
//    };
//    int dif = temp - prevTemp;
//    printf("%i %li convertT status %u readScratchpad status %u CRC %s temp %i.%s %c%i.%s\n",
//      i, osKernelSysTick(), unsigned(status), unsigned(status2), sp.Crc == sp.calcCrc() ? "ok" : "fail",
//      temp >> 4, frac[temp & 0xf], dif == 0 ? ' ' : dif > 0 ? '+' : '-', abs(dif) >> 4, frac[abs(dif) & 0xf]);
//    prevTemp = temp;
//  }

//  Sampler sampler;
  auto&& enc = Enc28j60::CreateDevice(std::unique_ptr<Enc28j60::Env>(), Enc28j60::CreateSpiStm32(SPI1, Pin::Def(GPIOD, GPIO_PIN_7, true)), {});
  enc->test();

  for (;;)
  {
    Tools::IdleMeasure im;

    Os::Thread::delay(1000);
//    Rt::stall(SystemCoreClock);

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
