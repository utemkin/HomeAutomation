#include <common/utils.h>
#include <common/stm32.h>
#include <enc28j60/enc28j60spistm32.h>
#include <enc28j60/enc28j60lwip.h>
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

void testAdc()
{
  __HAL_RCC_ADC3_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_ADC3_FORCE_RESET();
  __HAL_RCC_ADC3_RELEASE_RESET();

  ADC_TypeDef* const adc = ADC3;
  DMA_Channel_TypeDef* const dma = DMA2_Channel5;
  
  dma->CPAR = uint32_t(&adc->DR);
  dma->CMAR = uint32_t(&s_buf[7 + 7]);
  dma->CNDTR = 6;
  dma->CCR = DMA_CCR_PL_0 | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC | DMA_CCR_EN;
  
  adc->CR1 = ADC_CR1_SCAN;
  adc->CR2 = ADC_CR2_DMA | ADC_CR2_ADON;
  RT::stall(72); //fixme: wait Tstab=1uS
  adc->SQR1 = ((6 - 1) << ADC_SQR1_L_Pos);
  adc->SQR3 = (ADC_CHANNEL_13 << ADC_SQR3_SQ6_Pos) | 
              (ADC_CHANNEL_12 << ADC_SQR3_SQ5_Pos) | 
              (ADC_CHANNEL_11 << ADC_SQR3_SQ4_Pos) | 
              (ADC_CHANNEL_10 << ADC_SQR3_SQ3_Pos) | 
              (ADC_CHANNEL_1 << ADC_SQR3_SQ2_Pos) | 
              (ADC_CHANNEL_0 << ADC_SQR3_SQ1_Pos);
  adc->CR2 = ADC_CR2_DMA | ADC_CR2_ADON;
  OS::Thread::delay(1000);
  dma->CCR = 0;

  dma->CNDTR = 6;
  dma->CCR = DMA_CCR_PL_0 | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC | DMA_CCR_EN;
  adc->CR2 = ADC_CR2_DMA | ADC_CR2_ADON;
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
  testAdc();

  for(;;)
  {
    Tools::IdleMeasure im;
    OS::Thread::delay(1000);
    int percent;
    int hundreds;
    im.get(percent, hundreds);
    printf("CPU IDLE=%02u.%02u%%\n", percent, hundreds);
  }
}
