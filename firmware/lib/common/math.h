#pragma once

#include <lib/common/utils.h>
#include <limits>

namespace math
{
  template<int shift, unsigned lowerPercent, unsigned upperPercent>
  class BounceFilter : mstd::noncopyable
  {
  public:
    bool next(bool const bit)
    {
      auto acc = m_acc;

      acc -= acc >> shift;
      if (bit)
        acc += c_range >> shift;

      m_acc = acc;

      if (acc < c_range / 100u * lowerPercent)
      {
        if (m_state)
        {
          m_state = false;
          return true;
        }
      }
      else if (acc > c_range / 100u * upperPercent)
      {
        if (!m_state)
        {
          m_state = true;
          return true;
        }
      }

      return false;
    }

    bool getState() const
    {
      return m_state;
    }
  
  protected:
    unsigned m_acc = 0;
    constexpr static auto c_range = std::numeric_limits<decltype(m_acc)>::max();
    bool m_state = false;
  };
}
