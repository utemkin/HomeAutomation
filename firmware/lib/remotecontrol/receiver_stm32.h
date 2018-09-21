#pragma once

#include <lib/remotecontrol/receiver.h>
#include <lib/common/pin_stm32.h>
#include <lib/common/stm32.h>
#include <memory>

namespace RC
{
  std::unique_ptr<Receiver> CreateReceiverStm32(TIM_TypeDef* tim, const Pin::Def& pin);
}
