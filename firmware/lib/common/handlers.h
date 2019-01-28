#pragma once

#include <lib/common/hal.h>
#include <lib/common/utils.h>

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
    //must be ready to be called from handler context
    using Callback = mstd::Callback<bool, Hal::Irq>;

  public:
    Handler(Callback&& callback)
      : m_callback(std::move(callback))
    {
    }

    ~Handler()
    {
      Rt::fatal();
    }

    // instance of Handler can only be installed once
    void install(Hal::Irq irq);

  protected:
    Handler* m_next = nullptr;
    Callback const m_callback;

  friend class Vectors;
  };

  class SemaphoreHandler : public Handler, public Os::BinarySemaphore
  {
  public:
    SemaphoreHandler(Callback&& callback)
      : Handler(std::move(callback))
    {
    }
  };

  class SignalingHandler : public Handler
  {
  public:
    SignalingHandler(Callback&& callback)
      : Handler(std::move(callback))
    {
    }

    void signal()
    {
      if (m_threadId == NULL)
        Rt::fatal();

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
