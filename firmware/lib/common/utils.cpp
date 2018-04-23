#include <common/utils.h>

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

  void IdleMeasure::get(int& percent, int& hundreds)
  {
    update();
    {
      OS::CriticalSection cs;
      uint32_t p = m_value * 10000 / m_total;
      percent = p / 100;
      hundreds = p % 100;
    }
  }
}
