#include <common/utils.h>

namespace
{
  volatile uint32_t s_value = 0;
  uint32_t s_calValue = 0;
  uint32_t s_calTotal = 0;

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

  void IdleMeasure::get(int& percent, int& hundreds)  //fixme: this measurement method doesn't account for time spent in interrupts
  {
    update();
    {
      OS::CriticalSection cs;
      uint32_t p = m_value * 10000 / m_total;
      percent = p / 100;
      hundreds = p % 100;
    }
  }

  void IdleMeasure2::update()
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

  void IdleMeasure2::get(int& percent, int& hundreds)
  {
    update();
    {
      OS::CriticalSection cs;
      uint32_t p = ((m_value * 10000 + m_total /2 ) / m_total * s_calTotal + s_calValue / 2) / s_calValue;
      if (p > 10000)
        p = 10000;
      percent = p / 100;
      hundreds = p % 100;
    }
  }

  void IdleMeasure2::calibrate()
  {
    Sample s1, s2, s3;
    OS::Thread::delay(1);
    sample(s1);
    OS::Thread::delay(1);
    sample(s2);
    OS::Thread::delay(2);
    sample(s3);
    s_calValue = s3.value - s2.value * 2 + s1.value;
    s_calTotal = (s3.total - s1.total + 1) / 3;
  }

  void IdleMeasure2::sample(Sample& s)
  {
    OS::InterruptDisabler id;
    s.value = s_value;
    s.total = portGET_RUN_TIME_COUNTER_VALUE();
  }
}
