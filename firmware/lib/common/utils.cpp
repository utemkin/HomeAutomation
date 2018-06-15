#include <lib/common/utils.h>

namespace
{
  volatile uint32_t s_value = 0;
  unsigned s_cal = 0;

  constexpr unsigned calibrate(uint64_t const value, uint64_t const total)
  {
    return unsigned(mstd::ridiv(value << 16, total));
  }

  extern "C" ATTR_SUPER_OPTIMIZE void vApplicationIdleHook()
  {
    for (;;)
      ++s_value;
  }
}

namespace Tools
{
  void CRC32::update(uint8_t data)
  {
    uint32_t crc = m_crc;
    crc = crc ^ data;
    crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
    crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
    crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
    crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
    crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
    crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
    crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
    crc = ((crc & 1) ? 0xedB88320 : 0) ^ (crc >> 1);
    m_crc = crc;
  }

  void IdleMeasure::update()
  {
    Sample next;
    sample(next);
    {
      OS::CriticalSection cs;
      m_value += next.value - m_previous.value;
      m_total += next.total - m_previous.total;
      m_previous = next;
    }
  }

  unsigned IdleMeasure::get(unsigned* tenths)
  {
    update();
    {
      OS::CriticalSection cs;
      unsigned const p = mstd::ridiv(::calibrate(m_value, m_total) * 1000u, s_cal);
      if (tenths)
      {
        *tenths = p % 10u;
        return  p / 10u;
      }

      return mstd::ridiv(p, 10u);
    }
  }

  void IdleMeasure::calibrate()
  {
    Sample s1, s2, s3;
    OS::Thread::delay(1);
    sample(s1);
    OS::Thread::delay(1);
    sample(s2);
    OS::Thread::delay(10);
    sample(s3);

    unsigned const value = s3.value - (s2.value << 1) + s1.value;
    unsigned const total = s3.total - (s2.total << 1) + s1.total;
    s_cal = ::calibrate(value, total);
  }

  void IdleMeasure::sample(Sample& s)
  {
    OS::InterruptDisabler id;
    s.value = s_value;
    s.total = DWT->CYCCNT;
  }
}
