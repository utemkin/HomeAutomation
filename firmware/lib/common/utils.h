#pragma once

#include "cmsis_os.h"
#include <atomic>
#include <limits>
#include <memory>

#define ATTR_OPTIMIZE __attribute__((optimize("-O2")))
#define ATTR_SUPER_OPTIMIZE __attribute__((optimize("-O3"))) __attribute__((aligned(8)))

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

  //provides correct rounding for unsigned integers
  template<typename T>
  constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type ridiv(T const v1, T const v2)
  {
    return (v1 + (v2 >> 1)) / v2;
  }

  //provides correct rounding for signed integers
  template<int shift, typename T>
  constexpr typename std::enable_if<std::is_signed<T>::value, T>::type rsar(T const v)
  {
    using UT = typename std::make_unsigned<T>::type;
    UT const offset = std::numeric_limits<UT>::max() / 2 + 1;
    return T((UT(v) + offset + (UT(1) << (shift - 1))) >> shift) - T(offset >> shift);
  }

  //provides correct rounding for unsigned integers
  template<int shift, typename T>
  constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type rsar(T const v)
  {
    return (v + (T(1) << (shift - 1))) >> shift;
  }

  template<typename T>
  constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type badd(T const v1, T const v2, T const limit = std::numeric_limits<T>::max())
  {
    return v1 < limit - v2 ? v1 + v2 : limit;
  }

  template<typename Ret, typename ...Args>
  class Callback
  {
  public:
    Callback() = default;

    template<Ret(*func)(Args...)>
    static Callback make()
    {
      return Callback(&Callback::fn<func>);
    }

    template<typename T, Ret(T::*func)(Args...)>
    static Callback make(T& obj)
    {
      return Callback(&Callback::fn<T, func>, &obj);
    }

    operator bool() const
    {
      return !!m_func;
    }

    Ret operator()(Args... args) const
    {
      return m_func(m_ctx, args...);
    }

  protected:
    Ret(*m_func)(void*, Args...) = nullptr;
    void* m_ctx;

  protected:
    Callback(Ret(*func)(void*, Args...))
      : m_func(func)
    {
    }

    Callback(Ret(*func)(void*, Args...), void* ctx)
      : m_func(func)
      , m_ctx(ctx)
    {
    }

    template<Ret(*func)(Args...)>
    static Ret fn(void* /*ctx*/, Args ...args)
    {
      return func(args...);
    }

    template<typename T, Ret(T::*func)(Args...)>
    static Ret fn(void* ctx, Args ...args)
    {
      return (static_cast<T*>(ctx)->*func)(args...);
    }
  };

  template<typename T, size_t N>
  class NonlockedFifo : noncopyable
  {
  public:
    NonlockedFifo() = default;

    bool store(const T& value)
    {
      auto writeIndex = m_writeIndex.load(std::memory_order_acquire);
      auto oldWriteIndex = writeIndex;

      ++writeIndex;
      if (writeIndex >= m_buffer.size())
        writeIndex = 0;

      if (writeIndex == m_readIndex.load(std::memory_order_relaxed))
        return false;

      m_buffer[oldWriteIndex] = value;

      m_writeIndex.store(writeIndex, std::memory_order_release);
      return true;
    }

    bool load(T& value)
    {
      auto readIndex = m_readIndex.load(std::memory_order_acquire);

      if (readIndex == m_writeIndex.load(std::memory_order_relaxed))
        return false;

      value = m_buffer[readIndex];
    
      ++readIndex;
      if (readIndex >= m_buffer.size())
        readIndex = 0;

      m_readIndex.store(readIndex, std::memory_order_release);
      return true;
    }

  protected:
    std::array<T, N + 1> m_buffer;
    std::atomic<size_t> m_readIndex = {0};
    std::atomic<size_t> m_writeIndex = {0};
  };
}

namespace RT
{
  void stall(const unsigned cycles);
  uint32_t getUnique();

  class HiresTimer : mstd::noncopyable
  {
  public:
    //must be ready to be called from handler context
    using Callback = mstd::Callback<void>;

  public:
    virtual ~HiresTimer() = default;
    virtual void start(uint32_t hz) = 0;
  };
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

  friend class Lock<RecursiveMutex>;
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

  class Thread : mstd::noncopyable
  {
  public:
    Thread()
    {
      osThreadDef(thread, &func, osPriorityNormal, 1, 256);
      m_thread = osThreadCreate(osThread(thread), this);
    }
    virtual ~Thread()
    {
      osThreadTerminate(m_thread);
    }
    static void yield()
    {
      osThreadYield();
    }
    static void delay(const TickType_t ticks)
    {
      vTaskDelay(ticks);
    }

  protected:
    virtual void func() = 0;

  protected:
    osThreadId m_thread;

  protected:
    static void func(const void* arg)
    {
      ((Thread*)arg)->func();
      Error_Handler();          //fixme
    }
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

namespace Tools
{
  class CRC32
  {
  public:
    void update(uint8_t data);
    void update(const uint8_t* data, size_t size)
    {
      for(;size !=0; ++data, --size)
        update(*data);
    }
    uint32_t get() const
    {
      return ~m_crc;
    }
    static uint32_t calculate(const uint8_t* data, size_t size)
    {
      CRC32 crc;
      crc.update(data, size);
      return crc.get();
    }
  protected:
    uint32_t m_crc = -1;
  };

  class IdleMeasure
  {
  public:
    static void calibrate();
    IdleMeasure()
    {
      sample(m_previous);
    }
    void update();
    unsigned get(unsigned* tenths = nullptr);

  protected:
    struct Sample
    {
      uint32_t value;
      uint32_t total;
    };
    Sample m_previous;
    uint64_t m_value = 0;
    uint64_t m_total = 0;

  protected:
    static void sample(Sample& s);
  };
}
