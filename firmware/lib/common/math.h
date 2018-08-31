#pragma once

#include <limits>

namespace math
{
  template<typename T, int frame, int lower, int upper>
  class BounceFilter
  {
    static_assert(frame >= 1);
    static_assert(frame <= std::numeric_limits<T>::digits);
    static_assert(lower >= 1);
    static_assert(lower <= upper);
    static_assert(upper <= frame);

  public:
    BounceFilter() = default;

    bool next(bool const bit)
    {
      auto current = m_current;
      auto ones = m_ones;

      if (current & 1)
        --ones;

      current >>= 1;

      if (bit)
      {
        current |= T(1) << (frame - 1);
        ++ones;
      }

      m_current = current;
      m_ones = ones;

      if (ones < lower)
      {
        if (m_state)
        {
          m_state = false;
          return true;
        }
      }
      else if (ones >= upper)
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
    T m_current = 0;
    int m_ones = 0;
    bool m_state = false;
  };
}
