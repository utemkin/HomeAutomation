#pragma once

#include <common/stm32.h>
#include <common/utils.h>

namespace RT
{
  std::unique_ptr<HiresTimer> CreateHiresTimer(TIM_TypeDef *tim, HiresTimer::Callback&& callback);
}
