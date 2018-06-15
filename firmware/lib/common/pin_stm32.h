#pragma once

#include <common/stm32.h>

namespace Pin
{
  class Def
  {
  public:
    Def() = default;

    Def(GPIO_TypeDef* const gpio, uint16_t const pin, bool const invert)
      : m_gpio(gpio)
      , m_pin(pin)
      , m_invert(invert)
    {
    }

    operator bool() const
    {
      return !!m_gpio;
    }

    GPIO_TypeDef* gpio() const
    {
      return m_gpio;
    }

    uint16_t pin() const
    {
      return m_pin;
    }

    bool invert() const
    {
      return m_invert;
    }

    bool load(const char* name);

  protected:
    GPIO_TypeDef* m_gpio = nullptr;
    uint16_t m_pin;
    bool m_invert;
  };

  class In
  {
  public:
    In() = default;

    In(const Def& def)
    {
      if (def)
      {
        m_idr = &def.gpio()->IDR;
        m_mask = def.pin();
        m_compare = def.invert() ? 0 : def.pin();
      }
    }

    operator bool() const
    {
      return !!m_idr;
    }

    bool read() const
    {
      return (*m_idr & m_mask) == m_compare;
    }

  protected:
    const __IO uint32_t* m_idr = nullptr;
    uint16_t m_mask;
    uint16_t m_compare;
  };

  class Out
  {
  public:
    Out() = default;

    Out(const Def& def)
    {
      if (def)
      {
        m_bsrr = &def.gpio()->BSRR;
        m_active = def.invert() ? def.pin() << 16 : def.pin();
        m_passive = def.invert() ? def.pin() : def.pin() << 16;
      }
    }

    operator bool() const
    {
      return !!m_bsrr;
    }

    void toActive() const
    {
      *m_bsrr = m_active;
    }

    void toPassive() const
    {
      *m_bsrr = m_passive;
    }

  protected:
    __IO uint32_t* m_bsrr = nullptr;
    uint32_t m_active;
    uint32_t m_passive;
  };
}
