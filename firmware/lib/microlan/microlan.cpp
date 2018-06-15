#include <lib/microlan/microlan.h>
#include <algorithm>

namespace MicroLan
{
  uint_fast8_t crc8(const uint8_t* data, size_t count, uint_fast8_t crc)
  {
    static const uint_least8_t lo[16] = { 0x00,0x5e,0xbc,0xe2,0x61,0x3f,0xdd,0x83,0xc2,0x9c,0x7e,0x20,0xa3,0xfd,0x1f,0x41 };
    static const uint_least8_t hi[16] = { 0x00,0x9d,0x23,0xbe,0x46,0xdb,0x65,0xf8,0x8c,0x11,0xaf,0x32,0xca,0x57,0xe9,0x74 };

    for (; count != 0; --count)
    {
      uint_fast8_t t = *data ^ crc;
      crc = lo[t & 0x0f] ^ hi[t >> 4];
      ++data;
    }

    return crc;
  }

  RomCode::RomCode(uint_fast8_t family, uint_fast64_t serialNumber)
  {
    data[0] = family;
    uint_fast32_t lo = (uint_fast32_t)serialNumber;
    data[1] = lo;
    lo >>= 8;
    data[2] = lo;
    lo >>= 8;
    data[3] = lo;
    lo >>= 8;
    data[4] = lo;
    uint_fast16_t hi = (uint_fast16_t)(serialNumber >> 32);
    data[5] = hi;
    hi >>= 8;
    data[6] = hi;
    data[7] = calcCrc();
  }

  uint_fast64_t RomCode::serialNumber() const
  {
    uint_fast32_t lo = (data[1] | (data[2] << 8)) | ((data[3] << 16) | (data[4] << 24));
    uint_fast16_t hi = data[5] | (data[6] << 8);

    return lo | ((uint_fast64_t)hi << 32);
  }

  bool Bus::isSupported(const Options& options) const
  {
    const auto& caps = capabilities();

    if (options.overdrive && !caps.overdriveSupported)
      return false;
    
    switch (options.powerMode)
    {
    case PowerMode::None:
      return true;
    case PowerMode::StrongPoolup5V:
      return options.strengthMicroamps <= caps.strengthMicroampsStrongPoolup5V;
    case PowerMode::Pulse12V:
      return options.strengthMicroamps <= caps.strengthMicroampsPulse12V;
    case PowerMode::External5V:
      return options.strengthMicroamps <= caps.strengthMicroampsExternal5V;
    }
    
    return false;
  }

  Status Bus::read(uint8_t* data, size_t size, bool last)
  {
    Status ret;

    for (; size != 0; --size)
    {
      uint_fast8_t byte = 0;

      for (uint_fast8_t i = 8; i != 0; --i)
      {
        bool bit;

        if ((ret = read(bit, last && size == 1 && i == 1)) != Status::Success)
          return ret;

        byte >>= 1;
        if (bit)
          byte |= 0x80;
      }

      *data++ = byte;
    }
    
    return Status::Success;
  }

  Status Bus::write(const uint8_t* data, size_t size, bool last)
  {
    Status ret;

    for (; size != 0; --size)
    {
      uint_fast8_t byte = *data++;

      for (uint_fast8_t i = 8; i != 0; --i)
      {
        bool bit = !!(byte & 1);

        byte >>= 1;
        if ((ret = write(bit, last && size == 1 && i == 1)) != Status::Success)
          return ret;
      }
    }

    return Status::Success;
  }

  Status Device::readRom(Mac& mac, RomCode& romCode)
  {
    Status ret;
    uint8_t buf[std::max(size_t(1), sizeof(RomCode))];

    buf[0] = uint8_t(Command::ReadRom);
    if ((ret = mac.write(buf, 1)) != Status::Success)
      return ret;

    if ((ret = mac.read(buf, sizeof(RomCode))) != Status::Success)
      return ret;

    romCode = *(RomCode*)buf;
    return Status::Success;
  }

  Status Device::matchRom(Mac& mac, const RomCode& romCode)
  {
    Status ret;
    uint8_t buf[1 + sizeof(RomCode)];

    buf[0] = uint8_t(Command::MatchRom);
    *(RomCode*)(buf+1) = romCode;
    if ((ret = mac.write(buf, 1 + sizeof(RomCode))) != Status::Success)
      return ret;

    return Status::Success;
  }

  Status Device::skipRom(Mac& mac)
  {
    Status ret;
    uint8_t buf[1];

    buf[0] = uint8_t(Command::SkipRom);
    if ((ret = mac.write(buf, 1)) != Status::Success)
      return ret;

    return Status::Success;
  }

  Status Device::check(Bus& bus, const RomCode& romCode, Device::Command command)
  {
    Enumerator enumerator(bus, romCode, command);
    RomCode romCode2;

    return enumerator.next(romCode2);
  }

  Status Enumerator::next(RomCode& romCode)
  {
    if (m_lastStatus != Status::Success)
      return m_lastStatus;

    Mac mac(m_lastStatus, m_bus);
    if (m_lastStatus != Status::Success)
      return m_lastStatus;

    if ((m_lastStatus = mac.reset()) != Status::Success)
      return m_lastStatus;

    uint8_t buf[1];
    buf[0] = uint8_t(m_command);
    if ((m_lastStatus = mac.write(buf, 1)) != Status::Success)
      return m_lastStatus;

    int_fast8_t nextNode = -1;
    for (int_fast8_t index = 0; index < 64; ++index)
    {
      bool not0;
      if ((m_lastStatus = mac.read(not0)) != Status::Success)
        return m_lastStatus;

      bool not1;
      if ((m_lastStatus = mac.read(not1)) != Status::Success)
        return m_lastStatus;

      bool bit;

      if (index < m_lastNode)
      {
        bit = m_lastRomCode.bit(index);
        if (bit)
        {
          if (not1)
          {
            m_lastStatus = Status::SearchFailed;
            return m_lastStatus;
          };
        }
        else
        {
          if (not0)
          {
            if (not1)
            {
              m_lastStatus = Status::SearchFailed;
              return m_lastStatus;
            };
            bit = true;
            m_lastNode = -1;
          }
        }
      }
      else if (index == m_lastNode)
      {
        if (not1)
        {
          m_lastStatus = Status::SearchFailed;
          return m_lastStatus;
        }

        bit = true;
      }
      else
      {
        if (not0 && not1)
        {
          m_lastStatus = Status::SearchFailed;
          return m_lastStatus;
        }

        bit = not0;
      }

      if (!bit && !not1)
        nextNode = index;

      m_lastRomCode.setBit(index, bit);
      if ((m_lastStatus = mac.write(bit)) != Status::Success)
        return m_lastStatus;
    }

    if (nextNode == -1)
      m_lastStatus = Status::NoDevices;
    else
      m_lastNode = nextNode;

    romCode = m_lastRomCode;

    return Status::Success;
  }

  Status DS18B20::Device::convertT(Mac& mac, unsigned timeoutMicroseconds)
  {
    switch (mac.options().powerMode)
    {
    case PowerMode::StrongPoolup5V:
      if (timeoutMicroseconds)
        return Status::NotSupported;
      break;
    case PowerMode::External5V:
      break;
    default:
      return Status::NotSupported;
    }

    Status ret;
    uint8_t buf[1];

    buf[0] = uint8_t(Command::ConvertT);
    if ((ret = mac.write(buf, 1, true)) != Status::Success)
      return ret;

    if (timeoutMicroseconds)
    {
      for (;;)
      {
        bool bit;
        
        if ((ret = mac.read(bit)) != Status::Success)
          return ret;

        if (bit)
          break;

        //fixme: check for imeout
        //if ()
        //  return Status::Timeout;
      }
    }

    return Status::Success;
  }

  Status DS18B20::Device::writeScratchpad(Mac& mac, const Scratchpad& scratchpad)
  {
    Status ret;
    uint8_t buf[1 + sizeof(scratchpad)];

    buf[0] = uint8_t(Command::WriteScratchpad);
    *(reinterpret_cast<Scratchpad*>(buf + 1)) = scratchpad;
    
    if ((ret = mac.write(buf, 1 + sizeof(scratchpad), true)) != Status::Success)
      return ret;

    return Status::Success;
  }

  Status DS18B20::Device::readScratchpad(Mac& mac, Scratchpad& scratchpad)
  {
    Status ret;
    uint8_t buf[std::max(size_t(1), sizeof(scratchpad))];

    buf[0] = uint8_t(Command::ReadScratchpad);
    if ((ret = mac.write(buf, 1)) != Status::Success)
      return ret;

    if ((ret = mac.read(buf, sizeof(scratchpad), true)) != Status::Success)
      return ret;
    scratchpad = *(reinterpret_cast<Scratchpad*>(buf));

    return Status::Success;
  }

  Status DS18B20::Device::copyScratchpad(Mac& mac)
  {
    Status ret;
    uint8_t buf[1];

    buf[0] = uint8_t(Command::CopyScratchpad);
    if ((ret = mac.write(buf, 1, true)) != Status::Success)
      return ret;

    return Status::Success;
  }

  Status DS18B20::Device::recallEE(Mac& mac, unsigned timeoutMicroseconds)
  {
    switch (mac.options().powerMode)
    {
    case PowerMode::StrongPoolup5V:
      if (timeoutMicroseconds)
        return Status::NotSupported;
      break;
    case PowerMode::External5V:
      break;
    default:
      return Status::NotSupported;
    }

    Status ret;
    uint8_t buf[1];

    buf[0] = uint8_t(Command::RecallEE);
    if ((ret = mac.write(buf, 1, true)) != Status::Success)
      return ret;

    if (timeoutMicroseconds)
    {
      for (;;)
      {
        bool bit;
        
        if ((ret = mac.read(bit)) != Status::Success)
          return ret;

        if (bit)
          break;

        //fixme: check for imeout
        //if ()
        //  return Status::Timeout;
      }
    }

    return Status::Success;
  }

  Status DS18B20::Device::readPowerSupply(Mac& mac, bool& externallyPowered)
  {
    Status ret;
    uint8_t buf[1];

    buf[0] = uint8_t(Command::ReadPowerSupply);
    if ((ret = mac.write(buf, 1)) != Status::Success)
      return ret;

    bool bit;
    if ((ret = mac.read(bit, true)) != Status::Success)
      return ret;
    
    externallyPowered = bit;
    return Status::Success;
  }
}
