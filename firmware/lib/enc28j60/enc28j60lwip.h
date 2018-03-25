#pragma once
#include <enc28j60/enc28j60.h>
#include <common/utils.h>
#include <memory>

class Enc28j60lwip : public Enc28j60Services
{
public:
  static void initLwip();
};

std::unique_ptr<Enc28j60lwip> CreateEnc28j60lwip();
