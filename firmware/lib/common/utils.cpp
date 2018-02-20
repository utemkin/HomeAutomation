#include "utils.h"
#include "stm32f1xx_hal.h"

namespace RT
{
#if 1
  // real measured delay min(24, [cycles, cycles + 7])
  void ATTR_SUPER_OPTIMIZE stall(const unsigned cycles)
  {
    const decltype(DWT->CYCCNT)* const cyccnt = &DWT->CYCCNT;
    const unsigned initial = *cyccnt - 23;
    while (*cyccnt - initial <= cycles);
  }
#else
  // real measured delay min(31, [cycles, cycles + 3])
  void ATTR_SUPER_OPTIMIZE stall(const unsigned cycles)
  {
    const decltype(DWT->CYCCNT)* const cyccnt = &DWT->CYCCNT;
    const unsigned initial = *cyccnt - 34;
    asm volatile(
    "   .align  3                 \n\t"
    "loop%=:                      \n\t"
    "   ldr     r3, [%[cyccnt]]   \n\t"
    "   subs    r3, %[initial]    \n\t"
    "   subs    r3, %[cycles]     \n\t"
    "   bls     loop%=            \n\t"
    "   cmp     r3, #10           \n\t"
    "   bge     exit%=            \n\t"
    "   cmp     r3, #8            \n\t"
    "   bge     exit%=            \n\t"
    "   cmp     r3, #6            \n\t"
    "   bge     exit%=            \n\t"
    "   cmp     r3, #4            \n\t"
    "   bge     exit%=            \n\t"
    "   cmp     r3, #2            \n\t"
    "   bge     exit%=            \n\t"
    "   nop                       \n\t"
    "   b       exit%=            \n\t"
    "   .align  2                 \n\t"
    "exit%=:                      \n\t"
    "                               " : : [cycles] "r" (cycles), [cyccnt] "r" (cyccnt), [initial] "r" (initial) : "cc", "r3"
    );
  }
#endif
/*
  portDISABLE_INTERRUPTS();
  DWT->CYCCNT = 0;
  for (int i = 0; i < 100; ++i)
  {
    unsigned i1 = DWT->CYCCNT;
    RT::stall(i);
    unsigned i2 = DWT->CYCCNT;
    printf("i=%u, CYCCNT=%u\n", i, i2 - i1);
  }
  portENABLE_INTERRUPTS();
*/
}
