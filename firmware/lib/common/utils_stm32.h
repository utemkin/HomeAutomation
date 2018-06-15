#pragma once

#include <lib/common/stm32.h>
#include <lib/common/utils.h>

namespace RT
{
  std::unique_ptr<HiresTimer> CreateHiresTimer(TIM_TypeDef *tim, HiresTimer::Callback&& callback);
}
