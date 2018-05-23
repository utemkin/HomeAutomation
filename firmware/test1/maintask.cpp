#include <common/utils.h>
#include <common/stm32.h>
#include <enc28j60/enc28j60spistm32.h>
#include <enc28j60/enc28j60lwip.h>
#include <analog/adcstm32.h>
#include "main.h"
#include "adc.h"

static uint16_t s_buf[7 + 7 + 6];

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{
  if (AdcHandle == &hadc3)
  {
    HAL_ADC_Stop_DMA(&hadc3);
    HAL_ADC_Start(&hadc2);
    HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t*)s_buf, 7);
  }
  else
  {
    HAL_ADCEx_MultiModeStop_DMA(&hadc1);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)(s_buf + 7 + 7), 6);
  }
}

extern "C" void maintask()
{
  uint32_t uid[3];
  HAL_GetUID(uid);
  printf("Device %08lx%08lx%08lx is running at %lu heap size=%u\n", uid[0], uid[1], uid[2], HAL_RCC_GetHCLKFreq(), xPortGetFreeHeapSize());

//  Enc28j60::LwipNetif::initLwip();
//  auto netif = Enc28j60::CreateLwipNetif(Enc28j60::CreateSpiStm32(SPI1, SPI1_CS_GPIO_Port, SPI1_CS_Pin, true));
//  netif->setDefault();
//  netif->startDhcp();

//  HAL_ADC_Start_DMA(&hadc3, (uint32_t*)(s_buf + 7 + 7), 6);

//  Tools::IdleMeasure::calibrate();

//  auto adc = Analog::CreateAdcStm32(SWITCH_ADC_GPIO_Port, SWITCH_ADC_Pin, false);
  auto adc = Analog::CreateAdcStm32(0, 0, false);

  uint32_t clk = DWT->CYCCNT;
  adc->convert();
//  OS::Thread::yield();
//  portYIELD();
  printf("%lu\n", DWT->CYCCNT - clk);

  for(;;)
  {
//    Tools::IdleMeasure im;
//
////    OS::Thread::delay(1000);
//
//    for(int i = 0; i < 10000; ++i)
//    {
//      adc->convert();
//    }
//
//    unsigned tenths;
//    printf("CPU IDLE=%02u.%01u%%\n", im.get(&tenths), tenths);
  }
}
