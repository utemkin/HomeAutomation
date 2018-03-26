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
  };

  std::unique_ptr<LwipNetif> CreateLwipNetif(std::unique_ptr<Spi>&& spi);
}
