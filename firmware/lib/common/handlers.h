#pragma once
#include "stm32f1xx_hal.h"

namespace Irq
{
  class Vectors
  {
  public:
    static inline void handle();

    Vectors() = delete;
  };

  class Handler
  {
  public:
    // instance of Handler can only be installed once
    void install(IRQn_Type IRQn);

  protected:
    Handler() = default;
    ~Handler()
    {
      Error_Handler();
    }
    virtual bool handle(IRQn_Type IRQn) = 0;

  protected:
    Handler* m_next = nullptr;

    Handler(const Handler&) = delete;
    Handler& operator =(const Handler&) = delete;

    friend class Vectors;
  };

  template<class P, class T, bool(T::*func)(IRQn_Type)>
  class DelegatedHandler : public P
  {
  public:
    DelegatedHandler(T* object)
      : m_object(object)
    {
    }

  protected:
    virtual bool handle(IRQn_Type IRQn) override
    {
      return (m_object->*func)(IRQn);
    }

  protected:
    T* m_object;
  };
}
