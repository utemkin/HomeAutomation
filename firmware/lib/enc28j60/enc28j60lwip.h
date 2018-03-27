#pragma once
#include <enc28j60/enc28j60.h>
#include <common/utils.h>
#include <memory>

namespace Enc28j60
{
  class LwipNetif : mstd::noncopyable
  {
  public:
    static void initLwip();
    virtual void setDefault() = 0;
    virtual void startDhcp() = 0;
    virtual void start(uint32_t ipaddr, uint32_t netmask, uint32_t gw) = 0;
    virtual void stop() = 0;
  };

  std::unique_ptr<LwipNetif> CreateLwipNetif(std::unique_ptr<Spi>&& spi);
}
