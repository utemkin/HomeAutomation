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
  class Lock : mstd::noncopyable
  {
  public:
    explicit Lock(T& obj)
      : m_obj(&obj)
    {
      m_obj->lock();
    }
    explicit Lock(T* obj)
      : m_obj(obj)
    {
      if (m_obj)
        m_obj->lock();
    }
    ~Lock()
    {
      if (m_obj)
        m_obj->unlock();
    }

  protected:
    T* m_obj;
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

    friend class Lock<Mutex>;
  };

  class RecursiveMutex : mstd::noncopyable
  {
  public:
    RecursiveMutex()
    {
      osMutexDef(mutex);
      m_mutex = osRecursiveMutexCreate(osMutex(mutex));
    }
    ~RecursiveMutex()
    {
      osMutexDelete(m_mutex);
    }

  protected:
    void lock()
    {
      osRecursiveMutexWait(m_mutex, osWaitForever);
    }
    void unlock()
    {
      osRecursiveMutexRelease(m_mutex);
    }

  protected:
    osMutexId m_mutex;

    friend class Lock<Mutex>;
  };

  class BinarySemaphore : mstd::noncopyable
  {
  public:
    BinarySemaphore()
    {
      osSemaphoreDef(semaphore);
      m_semaphore = osSemaphoreCreate(osSemaphore(semaphore), 1);
      wait(0);
    }
    ~BinarySemaphore()
    {
      osSemaphoreDelete(m_semaphore);
    }
    void signal()
    {
      osSemaphoreRelease(m_semaphore);
    }
    bool wait(uint32_t timeout = osWaitForever)    //returns false if timeout, true otherwise
    {
      return !!(osSemaphoreWait(m_semaphore, timeout) == osOK);
    }

  protected:
    osSemaphoreId m_semaphore;
  };

  class ExpirationTimer
  {
  public:
    constexpr static uint32_t tickPeriodus()
    {
      return uint32_t(1000000ul / configTICK_RATE_HZ);
    }
    constexpr static TickType_t usToTicks(const uint32_t us)
    {
      return TickType_t((us + tickPeriodus() - 1) / tickPeriodus());
    }
    static void delay(const TickType_t ticks)     // delay until at least ticks whole tick periods have passed
    {
      vTaskDelay(ticks + 1);
    }
    static void delayus(const uint32_t us)        // delay until at least us have passed
    {
      delay(usToTicks(us));
    }
    bool elapsed(const TickType_t ticks) const    // returns true if at least ticks whole tick periods have passed
    {
      return TickType_t(xTaskGetTickCount() - m_initial) > TickType_t(ticks);
    }
    bool elapsedus(const uint32_t us) const       // returns true if at least us have passed
    {
      return elapsed(usToTicks(us));
    }

  protected:
    TickType_t m_initial = xTaskGetTickCount();
  };
}
