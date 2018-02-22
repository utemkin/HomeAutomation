#pragma once
#include <common/utils.h>
#include <common/stm32.h>

namespace Irq
{
  class Vectors : mstd::noncopyable
  {
  public:
    static inline void handle();

    Vectors() = delete;
  };

  class Handler : mstd::noncopyable
  {
  public:
    // instance of Handler can only be installed once
    void install(IRQn_Type IRQn);

  protected:
    ~Handler()
    {
      Error_Handler();
    }
    virtual bool handle(IRQn_Type IRQn) = 0;

  protected:
    Handler* m_next = nullptr;

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
