#pragma once
#include "cmsis_os.h"

#define ATTR_OPTIMIZE __attribute__((optimize("-O2")))
#define ATTR_SUPER_OPTIMIZE __attribute__((optimize("-O3", "-falign-functions=8", "-falign-labels=8", "-falign-loops=8", "-falign-jumps=8")))
#define ATTR_NOINLINE __attribute__((noinline))
#define ATTR_FORCEINLINE __attribute__((always_inline))

namespace OS
{
  // Can be used recursively and/or from handler
  class InterruptDisabler
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

    InterruptDisabler(const InterruptDisabler&) = delete;
    InterruptDisabler& operator =(const InterruptDisabler&) = delete;
  };

  // Can be used recursively
  class CriticalSection
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

    CriticalSection(const CriticalSection&) = delete;
    CriticalSection& operator =(const CriticalSection&) = delete;
  };

  template<class T>
  class Locker
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

    Locker(const Locker&) = delete;
    Locker& operator =(const Locker&) = delete;
  };

  class Mutex
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

    Mutex(const Mutex&) = delete;
    Mutex& operator =(const Mutex&) = delete;

    friend class Locker<Mutex>;
  };
}

namespace RT
{
  void stall(const unsigned cycles);
}
