#pragma once
#include "cmsis_os.h"
#include <memory>

#define ATTR_OPTIMIZE __attribute__((optimize("-O2")))
#define ATTR_SUPER_OPTIMIZE __attribute__((optimize("-O3", "-falign-functions=8", "-falign-labels=8", "-falign-loops=8", "-falign-jumps=8")))
#define ATTR_NOINLINE __attribute__((noinline))
#define ATTR_FORCEINLINE __attribute__((always_inline))

namespace mstd
{
  template<typename T>
  std::shared_ptr<T> to_shared(std::unique_ptr<T>&& ptr)
  {
    return std::move(ptr);
  }

  class noncopyable
  {
  protected:
    noncopyable() = default;
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator =(const noncopyable&) = delete;
  };
}

namespace RT
{
  void stall(const unsigned cycles);
}

namespace OS
{
  // Can be used recursively and/or from handler
  class InterruptDisabler : mstd::noncopyable
  {
  public:
    InterruptDisabler()
    {
      m_oldMask = portSET_INTERRUPT_MASK_FROM_ISR();
    }
    ~InterruptDisabler()
    {
      portCLEAR_INTERRUPT_MASK_FROM_ISR(m_oldMask);
    }

  protected:
    uint32_t m_oldMask;
  };

  // Can be used recursively
  class CriticalSection : mstd::noncopyable
  {
  public:
    CriticalSection()
    {
      portENTER_CRITICAL();
    }
    ~CriticalSection()
    {
      portEXIT_CRITICAL();
    }
  };

  template<class T>
  class Locker : mstd::noncopyable
  {
  public:
    Locker(T& obj)
      : m_obj(obj)
    {
      m_obj.lock();
    }
    ~Locker()
    {
      m_obj.unlock();
    }

  protected:
    T& m_obj;
  };

  class Mutex : mstd::noncopyable
  {
  public:
    Mutex()
    {
      osMutexDef(mutex);
      m_mutex = osMutexCreate(osMutex(mutex));
    }
    ~Mutex()
    {
      osMutexDelete(m_mutex);
    }

  protected:
    void lock()
    {
      osMutexWait(m_mutex, osWaitForever);
    }
    void unlock()
    {
      osMutexRelease(m_mutex);
    }

  protected:
    osMutexId m_mutex;

    friend class Locker<Mutex>;
  };
}
