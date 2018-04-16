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
}
