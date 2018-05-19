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
    virtual ~Handler()
    {
      Error_Handler();
    }
    virtual bool handle(IRQn_Type IRQn) = 0;

  protected:
    Handler* m_next = nullptr;

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
