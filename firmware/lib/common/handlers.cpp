#include <common/handlers.h>
#include <common/utils.h>
#include <atomic>
#include <cstdint>

#ifndef NUM_HANDLERS

#  if defined(STM32F1)

#    define NUM_HANDLERS (68)

#  elif defined(STM32F4)

#    define NUM_HANDLERS (102)

#  elif defined(STM32F7)

#    define NUM_HANDLERS (110)

#  endif

#endif

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

static std::atomic<Irq::Handler*> volatile handlers[NUM_HANDLERS + 16];

void Irq::Handler::install(IRQn_Type IRQn)
{
  if (m_next)
    Error_Handler();

  IRQn = IRQn_Type(int(IRQn) + 16);
  assert_param(IRQn < NUM_HANDLERS + 16);
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
#if NUM_HANDLERS > 10
DECLARE10(1);
#endif
#if NUM_HANDLERS > 20
DECLARE10(2);
#endif
#if NUM_HANDLERS > 30
DECLARE10(3);
#endif
#if NUM_HANDLERS > 40
DECLARE10(4);
#endif
#if NUM_HANDLERS > 50
DECLARE10(5);
#endif
#if NUM_HANDLERS > 60
DECLARE10(6);
#endif
#if NUM_HANDLERS > 70
DECLARE10(7);
#endif
#if NUM_HANDLERS > 80
DECLARE10(8);
#endif
#if NUM_HANDLERS > 90
DECLARE10(9);
#endif
#if NUM_HANDLERS > 100
DECLARE10(10);
#endif
#if NUM_HANDLERS > 110
DECLARE10(11);
#endif
#if NUM_HANDLERS > 120
DECLARE10(12);
#endif
#if NUM_HANDLERS > 130
DECLARE10(13);
#endif
#if NUM_HANDLERS > 140
DECLARE10(14);
#endif
#if NUM_HANDLERS > 150
DECLARE10(15);
#endif
#if NUM_HANDLERS > 160
DECLARE10(16);
#endif
#if NUM_HANDLERS > 170
DECLARE10(17);
#endif
#if NUM_HANDLERS > 180
DECLARE10(18);
#endif
#if NUM_HANDLERS > 190
DECLARE10(19);
#endif
#if NUM_HANDLERS > 200
DECLARE10(20);
#endif
#if NUM_HANDLERS > 210
DECLARE10(21);
#endif
#if NUM_HANDLERS > 220
DECLARE10(22);
#endif
#if NUM_HANDLERS > 230
DECLARE10(23);
#endif
#if NUM_HANDLERS > 240
#  error Maximum supported NUM_HANDLERS is 240
#endif

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
#if NUM_HANDLERS > 10
  REF10(1),
#endif
#if NUM_HANDLERS > 20
  REF10(2),
#endif
#if NUM_HANDLERS > 30
  REF10(3),
#endif
#if NUM_HANDLERS > 40
  REF10(4),
#endif
#if NUM_HANDLERS > 50
  REF10(5),
#endif
#if NUM_HANDLERS > 60
  REF10(6),
#endif
#if NUM_HANDLERS > 70
  REF10(7),
#endif
#if NUM_HANDLERS > 80
  REF10(8),
#endif
#if NUM_HANDLERS > 90
  REF10(9),
#endif
#if NUM_HANDLERS > 110
  REF10(11),
#endif
#if NUM_HANDLERS > 120
  REF10(12),
#endif
#if NUM_HANDLERS > 130
  REF10(13),
#endif
#if NUM_HANDLERS > 140
  REF10(14),
#endif
#if NUM_HANDLERS > 150
  REF10(15),
#endif
#if NUM_HANDLERS > 160
  REF10(16),
#endif
#if NUM_HANDLERS > 170
  REF10(17),
#endif
#if NUM_HANDLERS > 180
  REF10(18),
#endif
#if NUM_HANDLERS > 190
  REF10(19),
#endif
#if NUM_HANDLERS > 200
  REF10(20),
#endif
#if NUM_HANDLERS > 210
  REF10(21),
#endif
#if NUM_HANDLERS > 220
  REF10(22),
#endif
#if NUM_HANDLERS > 230
  REF10(23),
#endif
};
