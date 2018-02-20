#include "handlers.h"
#include "utils.h"
#include <atomic>
#include <cstdint>

extern "C" uint8_t _estack;
extern "C" uint8_t _sidata;
extern "C" uint8_t _sdata;
extern "C" uint8_t _edata;
extern "C" uint8_t _sbss;
extern "C" uint8_t _ebss;

extern "C" void SystemInit();
extern "C" void __libc_init_array();
extern "C" int main();

extern "C" void Reset_Handler()
{
  const uint8_t* src;
  uint8_t* dst;

  for (src = &_sidata, dst = &_sdata; dst < &_edata; ++src, ++dst)
    *dst = *src;

  for (dst = &_sbss; dst < &_ebss; ++dst)
    *dst = 0;

  SystemInit();
  __libc_init_array();
  main();
}

static std::atomic<Irq::Handler*> volatile handlers[100];

void Irq::Handler::install(IRQn_Type IRQn)
{
  if (m_next)
    Error_Handler();

  IRQn = IRQn_Type(int(IRQn) + 16);
  assert_param(IRQn < 100);   //fixme
  m_next = handlers[IRQn].load(std::memory_order_relaxed);
  while (!handlers[IRQn].compare_exchange_weak(m_next, this, std::memory_order_release, std::memory_order_relaxed));
}

inline void ATTR_FORCEINLINE ATTR_SUPER_OPTIMIZE Irq::Vectors::handle()
{
  unsigned IRQn = __get_IPSR();
  Irq::Handler* next = handlers[IRQn].load(std::memory_order_acquire);
  bool handled = false;
  for(;next != 0; next = next->m_next)
    handled |= next->handle(IRQn_Type(IRQn));
  if (!handled)
    Error_Handler();
}

extern "C" void ATTR_NOINLINE ATTR_SUPER_OPTIMIZE Default_Handler()
{
  Irq::Vectors::handle();
}

#define DECLARE(name) extern "C" void __attribute__((weak, alias("Default_Handler"))) name()
#define DECLARE1(num) DECLARE(Irq ## num ## _Handler)
#define DECLARE10(decade) \
  DECLARE1(decade ## 0);   \
  DECLARE1(decade ## 1);   \
  DECLARE1(decade ## 2);   \
  DECLARE1(decade ## 3);   \
  DECLARE1(decade ## 4);   \
  DECLARE1(decade ## 5);   \
  DECLARE1(decade ## 6);   \
  DECLARE1(decade ## 7);   \
  DECLARE1(decade ## 8);   \
  DECLARE1(decade ## 9)

#define REF(name) (uint32_t)&name
#define REF1(num) REF(Irq ## num ## _Handler)
#define REF10(decade) \
  REF1(decade ## 0),   \
  REF1(decade ## 1),   \
  REF1(decade ## 2),   \
  REF1(decade ## 3),   \
  REF1(decade ## 4),   \
  REF1(decade ## 5),   \
  REF1(decade ## 6),   \
  REF1(decade ## 7),   \
  REF1(decade ## 8),   \
  REF1(decade ## 9)

DECLARE(NMI_Handler);
DECLARE(HardFault_Handler);
DECLARE(MemManage_Handler);
DECLARE(BusFault_Handler);
DECLARE(UsageFault_Handler);
DECLARE(SVC_Handler);
DECLARE(DebugMon_Handler);
DECLARE(PendSV_Handler);
DECLARE(SysTick_Handler);

DECLARE10();
DECLARE10(1);
DECLARE10(2);
DECLARE10(3);
DECLARE10(4);
DECLARE10(5);
DECLARE1(60);
DECLARE1(61);
DECLARE1(62);
DECLARE1(63);
DECLARE1(64);
DECLARE1(65);
DECLARE1(66);
DECLARE1(67);

uint32_t __attribute__((section(".isr_vector"))) vectors[] =
{
  (uint32_t)&_estack,
  REF(Reset_Handler),
  REF(NMI_Handler),
  REF(HardFault_Handler),
  REF(MemManage_Handler),
  REF(BusFault_Handler),
  REF(UsageFault_Handler),
  0,
  0,
  0,
  0,
  REF(SVC_Handler),
  REF(DebugMon_Handler),
  0,
  REF(PendSV_Handler),
  REF(SysTick_Handler),
  REF10(),
  REF10(1),
  REF10(2),
  REF10(3),
  REF10(4),
  REF10(5),
  REF1(60),
  REF1(61),
  REF1(62),
  REF1(63),
  REF1(64),
  REF1(65),
  REF1(66),
  REF1(67),
};
