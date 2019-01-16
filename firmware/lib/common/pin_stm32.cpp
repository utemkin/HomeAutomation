#include <lib/common/pin_stm32.h>

namespace Pin
{
  bool Def::load(const char* name)
  {
    GPIO_TypeDef* gpio;
    uint16_t pin;
    bool invert;

    if (*name == '~')
    {
      invert = true;
      ++name;
    }
    else
      invert = false;
  
    if (name[0] != 'P')
      return false;

    switch (name[1])
    {
#ifdef GPIOA
    case 'A':
      gpio = GPIOA;
      break;
#endif
#ifdef GPIOB
    case 'B':
      gpio = GPIOB;
      break;
#endif
#ifdef GPIOC
    case 'C':
      gpio = GPIOC;
      break;
#endif
#ifdef GPIOD
    case 'D':
      gpio = GPIOD;
      break;
#endif
#ifdef GPIOE
    case 'E':
      gpio = GPIOE;
      break;
#endif
#ifdef GPIOF
    case 'F':
      gpio = GPIOF;
      break;
#endif
#ifdef GPIOG
    case 'G':
      gpio = GPIOG;
      break;
#endif
#ifdef GPIOH
    case 'H':
      gpio = GPIOH;
      break;
#endif
#ifdef GPIOI
    case 'I':
      gpio = GPIOI;
      break;
#endif
#ifdef GPIOJ
    case 'J':
      gpio = GPIOJ;
      break;
#endif
#ifdef GPIOK
    case 'K':
      gpio = GPIOK;
      break;
#endif
    default:
      return false;
    }

    if (name[2] < '0' || name[2] > '9')
      return false;

    if (name[3] == '\0')
      pin = 1 << (name[2] - '0');
    else
      if (name[4] == '\0' && name[2] == '1' && name[3] >= '0' && name[3] <= '5')
        pin = 1 << (name[3] - '0' + 10);
      else
        return false;

    m_gpio = gpio;
    m_pin = pin;
    m_invert = invert;
    return true;
  }
}
