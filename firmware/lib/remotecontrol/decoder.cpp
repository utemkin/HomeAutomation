#include <lib/remotecontrol/decoder.h>

namespace RC
{
  namespace
  {
    // if (v1 / 1.5 <= v2 && v2 <= v1 * 1.5)
    // {
    //   v1 = avg(v1, v2);
    //   return true;
    // }
    // else
    //   return false;
    bool merge(Decoder::DurationUs& v1, Decoder::DurationUs const v2)
    {
  
//      printf("%u -> %u\n", v1, v2);

      if (v2 > v1)
      {
        Decoder::DurationUs const diff = v2 - v1;
        if (v1 >= (diff << 1))
        {
          v1 += mstd::rsar<1>(diff);
          return true;
        }
      }
      else
      {
        Decoder::DurationUs const diff = v1 - v2;
        if (v2 >= (diff << 1))
        {
          v1 -= mstd::rsar<1>(diff);
          return true;
        }
      }

      return false;
    }
  }

  bool Decoder::process(bool const bit, DurationUs const durationUs, Message& message)
  {
    if (bit == m_lastBit)
    {
      auto& d = bit ? m_lastCycle.oneDurationUs : m_lastCycle.zeroDurationUs;
      d = mstd::badd(d, durationUs);
      return false;
    }

    m_lastBit = bit;
    if (!bit)
    {
      m_lastCycle.zeroDurationUs = durationUs;
      return false;
    }

    auto const res = process(message);
    m_lastCycle.oneDurationUs = durationUs;
    m_lastCycle.zeroDurationUs = 0;
    return res;
  }

  bool Decoder::process(Message& message)
  {

//    printf("%hu %hu\n", m_lastCycle.oneDurationUs, m_lastCycle.zeroDurationUs);

    auto const isSync = mstd::badd(m_lastCycle.oneDurationUs, m_lastCycle.zeroDurationUs) >= c_syncThreshold;

    switch(m_phase)
    {
    case Phase::WaitFirstSync:
      if (!isSync)
        break;

      m_phase = Phase::WaitFirstData;
      break;

    case Phase::WaitFirstData:
      if (isSync)
        break;

      m_dataSize = 0;
      m_phase = Phase::ReceiveFirstData;
      //no break

    case Phase::ReceiveFirstData:
      if (isSync)
      {
        if (m_dataSize >= c_minData)
        {
          m_syncCount = 1;
          m_repeatCount = 1;
          m_phase = Phase::WaitNextData;
          break;
        }

        m_phase = Phase::WaitFirstData;
        break;
      }

      if (m_dataSize >= m_data.size())
      {
        m_phase = Phase::WaitFirstSync;
        break;
      }

      m_data[m_dataSize] = m_lastCycle;
      ++m_dataSize;
      break;

    case Phase::WaitNextData:
      if (isSync)
      {
        ++m_syncCount;
        if (m_syncCount > c_maxSyncCount)
        {

//          printf("->WaitFirstData\n");

          m_phase = Phase::WaitFirstData;
          break;
        }

        break;
      }

      m_dataPos = 0;
      m_phase = Phase::ReceiveNextData;
      //no break

    case Phase::ReceiveNextData:
      if (isSync)
      {
        if (m_dataPos != m_dataSize)
        {
          m_syncCount = 1;

//          printf("->WaitFirstData\n");

          m_phase = Phase::WaitFirstData;
          break;
        }

        ++m_repeatCount;

        printf("size: %u rep: %u\n", m_dataSize, m_repeatCount);

        m_phase = Phase::WaitNextData;

        //fixme: report
        message.repeatCount = m_repeatCount;
        message.size = m_dataSize;
        message.data = m_data.data();

        return true;
      }

      if (m_dataPos > m_dataSize)
      {

//        printf("->WaitFirstSync\n");

        m_phase = Phase::WaitFirstSync;
        break;
      }

      if (!merge(m_data[m_dataPos].oneDurationUs, m_lastCycle.oneDurationUs) ||
          !merge(m_data[m_dataPos].zeroDurationUs, m_lastCycle.zeroDurationUs))
      {

//        printf("->WaitFirstSync\n");

        if (m_dataSize == 65)
          printf("merge failed at %u %u,%u -> %u, %u\n", m_dataPos,
            m_data[m_dataPos].oneDurationUs, m_data[m_dataPos].zeroDurationUs, m_lastCycle.oneDurationUs, m_lastCycle.zeroDurationUs);

        m_phase = Phase::WaitFirstSync;
        break;
      }

      ++m_dataPos;
      break;
    }

    return false;
  }
}
