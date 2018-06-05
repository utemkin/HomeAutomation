#include <common/utils.h>
#include <common/handlers.h>
#include <common/stm32.h>
#include <enc28j60/enc28j60spistm32.h>
#include <enc28j60/enc28j60lwip.h>
#include <analog/adcstm32.h>
#include "main.h"
#include "adc.h"

uint32_t tm[10] = {};

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

class HiresTimer
{
public:
  HiresTimer(TIM_TypeDef *const tim, uint32_t const hz)
    : m_tim(tim)
    , m_handler(this)
  {
    uint32_t pclk;
    RCC_ClkInitTypeDef clk;
    uint32_t lat;
    HAL_RCC_GetClockConfig(&clk, &lat);
#if defined(STM32F1)
    if (false)
      ;
#  ifdef TIM1
    else if (m_tim == TIM1)
    {
      __HAL_RCC_TIM1_CLK_ENABLE();
      __HAL_RCC_TIM1_FORCE_RESET();
      __HAL_RCC_TIM1_RELEASE_RESET();
      pclk = HAL_RCC_GetPCLK2Freq();
      if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
        pclk <<= 1;
      m_handler.install(TIM1_UP_IRQn);
      HAL_NVIC_SetPriority(TIM1_UP_IRQn, 14, 0);
      HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
    }
#  endif
#  ifdef TIM2
    else if (m_tim == TIM2)
    {
      __HAL_RCC_TIM2_CLK_ENABLE();
      __HAL_RCC_TIM2_FORCE_RESET();
      __HAL_RCC_TIM2_RELEASE_RESET();
      pclk = HAL_RCC_GetPCLK1Freq();
      if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
        pclk <<= 1;
      m_handler.install(TIM2_IRQn);
      HAL_NVIC_SetPriority(TIM2_IRQn, 14, 0);
      HAL_NVIC_EnableIRQ(TIM2_IRQn);
    }
#  endif
#  ifdef TIM3
    else if (m_tim == TIM3)
    {
      __HAL_RCC_TIM3_CLK_ENABLE();
      __HAL_RCC_TIM3_FORCE_RESET();
      __HAL_RCC_TIM3_RELEASE_RESET();
      pclk = HAL_RCC_GetPCLK1Freq();
      if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
        pclk <<= 1;
      m_handler.install(TIM3_IRQn);
      HAL_NVIC_SetPriority(TIM3_IRQn, 14, 0);
      HAL_NVIC_EnableIRQ(TIM3_IRQn);
    }
#  endif
#  ifdef TIM4
    else if (m_tim == TIM4)
    {
      __HAL_RCC_TIM4_CLK_ENABLE();
      __HAL_RCC_TIM4_FORCE_RESET();
      __HAL_RCC_TIM4_RELEASE_RESET();
      pclk = HAL_RCC_GetPCLK1Freq();
      if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
        pclk <<= 1;
      m_handler.install(TIM4_IRQn);
      HAL_NVIC_SetPriority(TIM4_IRQn, 14, 0);
      HAL_NVIC_EnableIRQ(TIM4_IRQn);
    }
#  endif
#  ifdef TIM5
    else if (m_tim == TIM5)
    {
      __HAL_RCC_TIM5_CLK_ENABLE();
      __HAL_RCC_TIM5_FORCE_RESET();
      __HAL_RCC_TIM5_RELEASE_RESET();
      pclk = HAL_RCC_GetPCLK1Freq();
      if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
        pclk <<= 1;
      m_handler.install(TIM5_IRQn);
      HAL_NVIC_SetPriority(TIM5_IRQn, 14, 0);
      HAL_NVIC_EnableIRQ(TIM5_IRQn);
    }
#  endif
#  ifdef TIM6
    else if (m_tim == TIM6)
    {
      __HAL_RCC_TIM6_CLK_ENABLE();
      __HAL_RCC_TIM6_FORCE_RESET();
      __HAL_RCC_TIM6_RELEASE_RESET();
      pclk = HAL_RCC_GetPCLK1Freq();
      if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
        pclk <<= 1;
      m_handler.install(TIM6_IRQn);
      HAL_NVIC_SetPriority(TIM6_IRQn, 14, 0);
      HAL_NVIC_EnableIRQ(TIM6_IRQn);
    }
#  endif
#  ifdef TIM7
    else if (m_tim == TIM7)
    {
      __HAL_RCC_TIM7_CLK_ENABLE();
      __HAL_RCC_TIM7_FORCE_RESET();
      __HAL_RCC_TIM7_RELEASE_RESET();
      pclk = HAL_RCC_GetPCLK1Freq();
      if (clk.APB1CLKDivider != RCC_CFGR_PPRE1_DIV1)
        pclk <<= 1;
      m_handler.install(TIM7_IRQn);
      HAL_NVIC_SetPriority(TIM7_IRQn, 14, 0);
      HAL_NVIC_EnableIRQ(TIM7_IRQn);
    }
#  endif
#  ifdef TIM8
    else if (m_tim == TIM8)
    {
      __HAL_RCC_TIM8_CLK_ENABLE();
      __HAL_RCC_TIM8_FORCE_RESET();
      __HAL_RCC_TIM8_RELEASE_RESET();
      pclk = HAL_RCC_GetPCLK2Freq();
      if (clk.APB2CLKDivider != RCC_CFGR_PPRE2_DIV1)
        pclk <<= 1;
      m_handler.install(TIM8_UP_IRQn);
      HAL_NVIC_SetPriority(TIM8_UP_IRQn, 14, 0);
      HAL_NVIC_EnableIRQ(TIM8_UP_IRQn);
    }
#  endif
#else
#  error Unsupported architecture
#endif
    else
    {
      //fixme
      return;
    }

    uint32_t div = pclk / hz;
    uint32_t psc = 1;
    while (div > 0x10000)
    {
      div >>= 1;
      psc <<= 1;
    }

    m_tim->DIER = TIM_DIER_UIE;
    m_tim->PSC = psc - 1;
    m_tim->CR1 = TIM_CR1_URS;
    m_tim->EGR = TIM_EGR_UG;
    m_tim->ARR = div - 1;

    tm[0] = DWT->CYCCNT;

    m_tim->CR1 = TIM_CR1_URS | TIM_CR1_CEN;
  }

  bool handle(IRQn_Type)
  {
    uint16_t const sr = m_tim->SR;
    if (sr & TIM_SR_UIF)
    {
      m_tim->SR = sr & ~TIM_SR_UIF;
//      m_handlerDmaTx.signal();    //distinguish success and error

      if (tm[1] == 0)
        tm[1] = DWT->CYCCNT;
      else if (tm[2] == 0)
        tm[2] = DWT->CYCCNT;
      else if (tm[3] == 0)
        tm[3] = DWT->CYCCNT;

      return true;
    }

    return false;
  }

protected:
      TIM_TypeDef* const m_tim;
      Irq::DelegatedHandler<Irq::Handler, HiresTimer, &HiresTimer::handle> m_handler;
};

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

  Tools::IdleMeasure::calibrate();

//  auto adc = Analog::CreateAdcStm32(SWITCH_ADC_GPIO_Port, SWITCH_ADC_Pin, false);
//  auto adc = Analog::CreateAdcStm32(0, 0, false);

//  uint32_t clk = DWT->CYCCNT;
//  adc->convert();
////  OS::Thread::yield();
////  portYIELD();
//  printf("%lu\n", DWT->CYCCNT - clk);

  HiresTimer ht(TIM7, 1000);
  OS::Thread::delay(1000);

  for(;;)
  {
    Tools::IdleMeasure im;

    OS::Thread::delay(1000);

//    for(int i = 0; i < 10000; ++i)
//    {
//      adc->convert();
//    }

    unsigned tenths;
    printf("CPU IDLE=%02u.%01u%%\n", im.get(&tenths), tenths);
  }
}
