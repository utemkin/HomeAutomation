#pragma once

#if defined(STM32F100xB) || defined(STM32F100xE) || defined(STM32F101x6) || \
    defined(STM32F101xB) || defined(STM32F101xE) || defined(STM32F101xG) || defined(STM32F102x6) || defined(STM32F102xB) || defined(STM32F103x6) || \
    defined(STM32F103xB) || defined(STM32F103xE) || defined(STM32F103xG) || defined(STM32F105xC) || defined(STM32F107xC)

#  include "stm32f1xx_hal.h"

#elif defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx) || \
      defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx) || \
      defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F410Tx) || defined(STM32F410Cx) || \
      defined(STM32F410Rx) || defined(STM32F411xE) || defined(STM32F446xx) || defined(STM32F469xx) || \
      defined(STM32F479xx) || defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
      defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx)

#  include "stm32f4xx_hal.h"

#elif defined(STM32F756xx) || defined(STM32F746xx) || defined(STM32F745xx) || defined(STM32F767xx) || \
      defined(STM32F769xx) || defined(STM32F777xx) || defined(STM32F779xx) || defined(STM32F722xx) || \
      defined(STM32F723xx) || defined(STM32F732xx) || defined(STM32F733xx)

#  include "stm32f7xx_hal.h"

#else

#  error STM32Fxxxxx must be defined

#endif
