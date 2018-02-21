#include <common/stm32.h>

extern "C" void maintask()
{
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Device %08lx%08lx%08lx is running at %lu\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq());

  for(;;);
}
