#pragma once

#include <common/utils.h>
#include <cstddef>
#include <cstdint>

namespace MicroLan
{
  enum class Status : uint_fast8_t
  {
    Success = 0,
    NotSupported,
    Timeout,
    Overcurrent,
    NoDevices,
    SearchFailed,
    BusShortToVdd,
    BusShortToGnd,
  };

  uint_fast8_t crc8(const uint8_t* data, size_t count, uint_fast8_t crc = 0);

  struct RomCode
  {
    uint8_t data[8];

    RomCode() = default;
    RomCode(uint_fast8_t family, uint_fast64_t serialNumber);
    uint_fast8_t family() const
    {
      return data[0];
    }
    uint_fast64_t serialNumber() const;
    uint_fast8_t crc() const
    {
      return data[7];
    }
    uint_fast8_t calcCrc() const
    {
      return crc8(data, sizeof(data) - 1);
    }
    bool bit(int_fast8_t index) const
    {
      return !!(data[index >> 3] & (1 << (index & 0x7)));
    }
    void setBit(int_fast8_t index, bool bit)
    {
      if (bit)
        data[index >> 3] |= 1 << (index & 0x7);
      else
        data[index >> 3] &= ~(1 << (index & 0x7));
    }
  };

  enum class PowerMode : uint_least8_t
  {
    None,
    StrongPoolup5V,
    Pulse12V,
    External5V,
  };

  struct Capabilities
  {
    bool overdriveSupported;
    unsigned strengthMicroampsStrongPoolup5V;
    unsigned strengthMicroampsPulse12V;
    unsigned strengthMicroampsExternal5V;
  };

  struct Options
  {
    bool overdrive = false;
    PowerMode powerMode = PowerMode::None;
    unsigned strengthMicroamps;
    unsigned durationMicroseconds;
  };

  class Mac;
  class Bus : mstd::noncopyable
  {
  public:
    virtual ~Bus() = default;
    virtual Capabilities capabilities() const = 0;
    virtual bool isSupported(const Options& options) const;

  protected:
    Bus() = default;

    virtual Status lock(const Options& options)
    {
      if (!isSupported(options))
        return Status::NotSupported;

      m_options = options;
      return  Status::Success;
    }
    virtual void unlock()
    {
    }

    virtual Status reset(bool& presence) = 0;
    virtual Status read(bool& bit, bool last) = 0;
    virtual Status write(bool bit, bool last) = 0;

    virtual Status read(uint8_t* data, size_t size, bool last);
    virtual Status write(const uint8_t* data, size_t size, bool last);

    static constexpr unsigned toUnit(const unsigned intervalNs, const unsigned freqHz)
    {
      return (((freqHz + 500000U) / 1000000U) * intervalNs + 500U) / 1000U;
    }

  protected:
    Options m_options;

  protected:
    // per AN126
    static constexpr unsigned c_A =     6000;
    static constexpr unsigned c_odA =   1000;
    static constexpr unsigned c_B =    64000;
    static constexpr unsigned c_odB =   7500;
    static constexpr unsigned c_C =    60000;
    static constexpr unsigned c_odC =   7500;
    static constexpr unsigned c_D =    10000;
    static constexpr unsigned c_odD =   2500;
    static constexpr unsigned c_E =     9000;
    static constexpr unsigned c_odE =   1000;
    static constexpr unsigned c_F =    55000;
    static constexpr unsigned c_odF =   7000;
    static constexpr unsigned c_G =        0;
    static constexpr unsigned c_odG =   2500;
    static constexpr unsigned c_H =   480000;
    static constexpr unsigned c_odH =  70000;
    static constexpr unsigned c_I =    70000;
    static constexpr unsigned c_odI =   8500;
    static constexpr unsigned c_J =   410000;
    static constexpr unsigned c_odJ =  40000;

  friend class Mac;
  };

  class Mac : mstd::noncopyable
  {
  public:
    Mac(Status& status, Bus& bus, const Options& options = Options())
    {
      status = bus.lock(options);
      if (status != Status::Success)
        return;

      m_bus = &bus;
    }
    ~Mac()
    {
      if (m_bus)
        m_bus->unlock();
    }

    const Options& options() const
    {
      return m_bus->m_options;
    }

    Status reset()
    {
      Status ret;
      bool presence;

      if ((ret = m_bus->reset(presence)) != Status::Success)
        return ret;

      if (!presence)
        return Status::NoDevices;

      return Status::Success;
    }
    Status read(bool& bit, bool last = false)
    {
      return m_bus->read(bit, last);
    }
    Status write(bool bit, bool last = false)
    {
      return m_bus->write(bit, last);
    }

    Status read(uint8_t* data, size_t size, bool last = false)
    {
      return m_bus->read(data, size, last);
    }
    Status write(const uint8_t* data, size_t size, bool last = false)
    {
      return m_bus->write(data, size, last);
    }

  protected:
    Bus* m_bus = nullptr;
  };

  class Device
  {
  public:
    enum class Command : uint8_t
    {
      SearchRom = 0xf0,
      ReadRom = 0x33,
      MatchRom = 0x55,
      SkipRom = 0xcc,
      AlarmSearch = 0xec,
    };

  public:
    Device(Bus& bus, const RomCode& romCode)
      : m_bus(bus)
      , m_romCode(romCode)
    {
    }

    Bus& bus() const
    {
      return m_bus;
    }
    const RomCode& romCode() const
    {
      return m_romCode;
    }

    Status readRom(Mac& mac, RomCode& romCode);
    Status matchRom(Mac& mac, const RomCode& romCode);
    Status skipRom(Mac& mac);

    template<class D, typename Func, typename ...Args>
    static Status executeWithReadRom(D& device, const Options& options, Func func, RomCode& romCode, Args... args)
    {
      Status ret;
      Mac mac(ret, device.bus(), options);
      if (ret != Status::Success)
        return ret;

      if ((ret = mac.reset()) != Status::Success)
        return ret;

      if ((ret = device.readRom(mac, romCode)) != Status::Success)
        return ret;

      if ((ret = (device.*func)(mac, args...)) != Status::Success)
        return ret;

      return Status::Success;
    }

    template<class D, typename Func, typename ...Args>
    static Status executeWithMatchRom(D& device, const Options& options, Func func, Args... args)
    {
      Status ret;
      Mac mac(ret, device.bus(), options);
      if (ret != Status::Success)
        return ret;

      if ((ret = mac.reset()) != Status::Success)
        return ret;

      if ((ret = device.matchRom(mac, device.romCode())) != Status::Success)
        return ret;

      if ((ret = (device.*func)(mac, args...)) != Status::Success)
        return ret;

      return Status::Success;
    }

    template<class D, typename Func, typename ...Args>
    static Status executeWithSkipRom(D& device, const Options& options, Func func, Args... args)
    {
      Status ret;
      Mac mac(ret, device.bus(), options);
      if (ret != Status::Success)
        return ret;

      if ((ret = mac.reset()) != Status::Success)
        return ret;

      if ((ret = device.skipRom(mac)) != Status::Success)
        return ret;

      if ((ret = (device.*func)(mac, args...)) != Status::Success)
        return ret;

      return Status::Success;
    }

    static Status check(Bus& bus, const RomCode& romCode, Device::Command command = Device::Command::SearchRom);

  protected:
    Bus& m_bus;
    const RomCode m_romCode;
  };

  class Enumerator : mstd::noncopyable
  {
  public:
    explicit Enumerator(Bus& bus, Device::Command command = Device::Command::SearchRom)
      : m_bus(bus)
      , m_command(command)
      , m_lastNode(-1)
    {
    }

    Status next(RomCode& romCode);

  protected:
    Bus& m_bus;
    Device::Command m_command;
    Status m_lastStatus = Status::Success;
    int_fast8_t m_lastNode;
    RomCode m_lastRomCode;

  protected:
    Enumerator(Bus& bus, const RomCode& romCode, Device::Command command)
      : m_bus(bus)
      , m_command(command)
      , m_lastNode(64)
      , m_lastRomCode(romCode)
    {
    }

  friend class Device;
  };

  namespace DS18B20
  {
    struct Scratchpad
    {
      uint8_t TempLo;
      uint8_t TempHi;
      uint8_t Th;
      uint8_t Tl;
      uint8_t Config;
      uint8_t Reserved0;
      uint8_t Reserved1;
      uint8_t Reserved2;
      uint8_t Crc;

      uint_fast8_t calcCrc() const
      {
        return crc8((const uint8_t*)this, sizeof(*this) - 1);
      }
      int temp() const
      {
        return (*reinterpret_cast<const int8_t*>(&TempHi) << 8) + TempLo;
      }
    };

    class Device : public ::MicroLan::Device
    {
    public:
      enum class Command : uint8_t
      {
        ConvertT = 0x44,
        WriteScratchpad = 0x4e,
        ReadScratchpad = 0xbe,
        CopyScratchpad = 0x48,
        RecallEE = 0xb8,
        ReadPowerSupply = 0xb4,
      };

    public:
      Device(Bus& bus, const RomCode& romCode)
        : ::MicroLan::Device(bus, romCode)
      {
      }

      Status convertT(Mac& mac, unsigned timeoutMicroseconds);
      Status writeScratchpad(Mac& mac, const Scratchpad& scratchpad);
      Status readScratchpad(Mac& mac, Scratchpad& scratchpad);
      Status copyScratchpad(Mac& mac);
      Status recallEE(Mac& mac, unsigned timeoutMicroseconds);
      Status readPowerSupply(Mac& mac, bool& externallyPowered);
    };
  }

  /*
  struct BusTraits
  {
    static void out1();
    static void out0();
    static bool in();
  };
  Any method may be either static, member or virtual member
  */
  template <typename BusTraits, unsigned CPUCLK, unsigned strengthMicroampsExternal5V>
  class BitbangBus : public Bus
  {
  public:
    // expects pins to be already configured
    explicit BitbangBus(BusTraits& busTraits)
      : m_busTraits(busTraits)
    {
      m_busTraits.out1();
    }
    virtual Capabilities capabilities() const override
    {
      return {
        overdriveSupported : false,
        strengthMicroampsStrongPoolup5V : 0,
        strengthMicroampsPulse12V : 0,
        strengthMicroampsExternal5V : strengthMicroampsExternal5V,
      };
    }
  
  protected:
    BusTraits& m_busTraits;
    static constexpr unsigned c_A_units = toUnit(c_A, CPUCLK);
    static constexpr unsigned c_B_units = toUnit(c_B, CPUCLK);
    static constexpr unsigned c_C_units = toUnit(c_C, CPUCLK);
    static constexpr unsigned c_D_units = toUnit(c_D, CPUCLK);
    static constexpr unsigned c_E_units = toUnit(c_E, CPUCLK);
    static constexpr unsigned c_F_units = toUnit(c_F, CPUCLK);
    static constexpr unsigned c_G_units = toUnit(c_G, CPUCLK);
    static constexpr unsigned c_H_units = toUnit(c_H, CPUCLK);
    static constexpr unsigned c_I_units = toUnit(c_I, CPUCLK);
    static constexpr unsigned c_J_units = toUnit(c_J, CPUCLK);

  protected:
    virtual Status reset(bool& presence) override
    {
      RT::stall(c_G_units);
      {
        OS::InterruptDisabler di;
        m_busTraits.out0();
        RT::stall(c_H_units);
        if(m_busTraits.in())
        {
          return Status::BusShortToVdd;
        }
        m_busTraits.out1();
        RT::stall(c_I_units);
        presence=!m_busTraits.in();
      }
      RT::stall(c_J_units);
      if(!m_busTraits.in())
      {
        return Status::BusShortToGnd;
      }
      return Status::Success;
    }
    virtual Status read(bool& bit, bool /*last*/) override
    {
      {
        OS::InterruptDisabler di;
        m_busTraits.out0();
        RT::stall(c_A_units);
        if(m_busTraits.in())
        {
          return Status::BusShortToVdd;
        }
        m_busTraits.out1();
        RT::stall(c_E_units);
        bit=m_busTraits.in();
      }
      RT::stall(c_F_units);
      if(!m_busTraits.in())
      {
        return Status::BusShortToGnd;
      }
      return Status::Success;
    }
    virtual Status write(bool bit, bool /*last*/) override
    {
      if (bit)
      {
        {
          OS::InterruptDisabler di;
          m_busTraits.out0();
          RT::stall(c_A_units);
          if(m_busTraits.in())
          {
            return Status::BusShortToVdd;
          }
          m_busTraits.out1();
        }
        RT::stall(c_B_units);
        if(!m_busTraits.in())
        {
          return Status::BusShortToGnd;
        }
      }
      else
      {
        {
          OS::InterruptDisabler di;
          m_busTraits.out0();
          RT::stall(c_C_units);
          if(m_busTraits.in())
          {
            return Status::BusShortToVdd;
          }
          m_busTraits.out1();
        }
        RT::stall(c_D_units);
        if(!m_busTraits.in())
        {
          return Status::BusShortToGnd;
        }
      }
      return Status::Success;
    }
  };
}
