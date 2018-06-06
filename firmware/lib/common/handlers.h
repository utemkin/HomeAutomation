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
    ~Handler()
    {
      Error_Handler();
    }

    // instance of Handler can only be installed once
    void install(IRQn_Type IRQn, bool(*func)(void*, IRQn_Type IRQn), void* ctx);

    template<class T, bool(T::*func)(IRQn_Type IRQn)>
    void install(IRQn_Type IRQn, T& obj)
    {
      install(IRQn, &Handler::f<T, func>, &obj);
    }

  protected:
    template<class T, bool(T::*func)(IRQn_Type IRQn)>
    static bool f(void* ctx, IRQn_Type IRQn)
    {
      return (static_cast<T*>(ctx)->*func)(IRQn);
    }

  protected:
    Handler* m_next = nullptr;
    bool(*m_func)(void*, IRQn_Type IRQn);
    void* m_ctx;

  friend class Vectors;
  };

  class SemaphoreHandler : public Handler, public OS::BinarySemaphore
  {
  };

  class SignalingHandler : public Handler
  {
  public:
    void signal()
    {
      if (m_threadId == NULL)
        Error_Handler();

      osSignalSet(m_threadId, 1);
    }

  protected:
    osThreadId m_threadId = NULL;

  friend class SignalingWaiter;
  };

  class SignalingWaiter : mstd::noncopyable
  {
  public:
    SignalingWaiter(SignalingHandler& handler)
      : m_handler(handler)
    {
      m_handler.m_threadId = osThreadGetId();
    }
    ~SignalingWaiter()
    {
      m_handler.m_threadId = NULL;
    }

    bool wait(uint32_t timeout = osWaitForever)    //returns false if timeout, true otherwise
    {
      return !(osSignalWait(1, timeout).status == osEventTimeout);
    }

  protected:
    SignalingHandler& m_handler;
  };
}
