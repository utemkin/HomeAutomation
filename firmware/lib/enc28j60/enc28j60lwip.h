#pragma once
#include <enc28j60/enc28j60.h>
#include <common/utils.h>
#include <memory>

namespace Enc28j60
{
  class LwipNetif : public Env
  {
  public:
    static void initLwip();
  };

  std::unique_ptr<LwipNetif> CreateLwipNetif();
}
