#include <common/utils.h>
#include <common/stm32.h>
#include <enc28j60/enc28j60spistm32.h>
#include <enc28j60/enc28j60lwip.h>
#include "main.h"

class IdleMeasure // : OS::Thread
{
//protected:
//  virtual void func() override
//  {
//    for (;;)
//    {
//      delay(100);
//    }
//  }

public:
  struct Sample
  {
    uint32_t value;
    uint32_t total;
  };
  void sample(Sample& s)
  {
    TaskStatus_t status;
    vTaskGetInfo(m_idleTask, &status, pdFALSE, eRunning);
    s.value = status.ulRunTimeCounter;
    s.total = portGET_RUN_TIME_COUNTER_VALUE();
  }

protected:
  const TaskHandle_t m_idleTask = xTaskGetIdleTaskHandle();
};

extern "C" void maintask()
{
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Device %08lx%08lx%08lx is running at %lu heap size=%u\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq(), xPortGetFreeHeapSize());
  
  IdleMeasure im;
  IdleMeasure::Sample first, second;
  im.sample(first);
  OS::Thread::delay(100);
  im.sample(second);
  printf("value %lu total %lu\n", second.value - first.value, second.total - first.total);

  Enc28j60::LwipNetif::initLwip();
  auto netif = Enc28j60::CreateLwipNetif(Enc28j60::CreateSpiStm32(SPI1, SPI1_CS_GPIO_Port, SPI1_CS_Pin, true));
  netif->setDefault();
  netif->startDhcp();

  for(;;)
    OS::Thread::delay(1000);
}
