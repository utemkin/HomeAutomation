#include <lib/common/utils.h>

extern "C" void maintask()
{
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Device %08lx%08lx%08lx is running at %lu heap size=%u\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq(), xPortGetFreeHeapSize());

  Tools::IdleMeasure::calibrate();

  for(;;)
  {
    Tools::IdleMeasure im;

    OS::Thread::delay(10);

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
