// ported from https://github.com/pimatic/RFControl

#pragma once

#include <lib/common/utils.h>
#include <cstddef>
#include <cstdint>

namespace RC
{
  class RFControl : mstd::noncopyable
  {
  public:
    using Sample = uint16_t;

  public:
    bool hasData() const
    {
      return m_data1Ready || m_data2Ready;
    }

    void getRaw(const Sample** const timingsUs, size_t* const timings_size)
    {
      if (m_data1Ready)
      {
        *timingsUs = &m_timingsUs[0];
        *timings_size = m_dataEnd[0] + 1;
        m_data1Ready = false;
      }
      else if (m_data2Ready)
      {
        *timingsUs = &m_timingsUs[m_dataStart[1]];
        *timings_size = m_dataEnd[1] - m_dataStart[1] + 1;
        m_data2Ready = false;
      }
    }

    void continueReceiving()
    {
      if(m_state == State::STATUS_RECORDING_END)
      {
        m_state = State::STATUS_WAITING;
        m_data1Ready = false;
        m_data2Ready = false;
      }
    }

    void process(Sample const durationUs)
    {
      switch (m_state)
      {
      case State::STATUS_WAITING:
        if (probablyFooter(durationUs))
          startRecording(durationUs);
        break;
      case State::STATUS_RECORDING_0:
        recording(durationUs, 0);
        break;
      case State::STATUS_RECORDING_1:
        recording(durationUs, 1);
        verification(1);
        break;
      case State::STATUS_RECORDING_2:
        recording(durationUs, 2);
        verification(2);
        break;
      case State::STATUS_RECORDING_3:
        recording(durationUs, 3);
        verification(3);
        break;
      case State::STATUS_RECORDING_END:
        break;
      }
    }

  protected:
    bool probablyFooter(Sample const durationUs) const
    {
      return durationUs >= c_minFooterUs; 
    }

    bool matchesFooter(Sample const durationUs) const
    {
      auto footerDeltaUs = m_footerUs/4;
      return m_footerUs - footerDeltaUs < durationUs && durationUs < m_footerUs + footerDeltaUs;
    }

    void startRecording(Sample const durationUs)
    {
      m_footerUs = durationUs;
      m_dataEnd[0] = 0;
      m_dataEnd[1] = 0;
      m_dataEnd[2] = 0;
      m_dataEnd[3] = 0;
      m_dataStart[0] = 0;
      m_dataStart[1] = 0;
      m_dataStart[2] = 0;
      m_dataStart[3] = 0;
      m_pack0EqualPack3 = true;
      m_pack1EqualPack3 = true;
      m_pack0EqualPack2 = true;
      m_pack1EqualPack2 = true;
      m_pack0EqualPack1 = true;
      m_data1Ready = false;
      m_data2Ready = false;
      m_state = State::STATUS_RECORDING_0;
    }

    void recording(Sample const durationUs, size_t const package)
    {
      if (matchesFooter(durationUs)) //test for footer (+-25%).
      {
        //Package is complete!!!!
        m_timingsUs[m_dataEnd[package]] = durationUs;
        m_dataStart[package + 1] = m_dataEnd[package] + 1;
        m_dataEnd[package + 1] = m_dataStart[package + 1];

        //Received more than 16 timings and start and end are the same footer then enter next state
        //less than 16 timings -> restart the package.
        if (m_dataEnd[package] - m_dataStart[package] >= 16)
        {
          if (m_state == State::STATUS_RECORDING_3)
            m_state = State::STATUS_RECORDING_END;
          else
            m_state = State(size_t(State::STATUS_RECORDING_0) + package + 1);
        }
        else
        {
          m_dataEnd[package] = m_dataStart[package];
          switch (package)
          {
            case 0:
              startRecording(durationUs); //restart
              break;
            case 1:
              m_pack0EqualPack1 = true;
              break;
            case 2:
              m_pack0EqualPack2 = true;
              m_pack1EqualPack2 = true;
              break;
            case 3:
              m_pack0EqualPack3 = true;
              m_pack1EqualPack3 = true;
              break;
          }
        }
      }
      else
      {
        //duration isnt a footer? this is the way.
        //if duration higher than the saved footer then the footer isnt a footer -> restart.
        if (durationUs > m_footerUs)
        {
          startRecording(durationUs);
        }
        //normal
        else if (m_dataEnd[package] < c_maxRecordings - 1)
        {
          m_timingsUs[m_dataEnd[package]] = durationUs;
          m_dataEnd[package]++;
        }
        //buffer reached end. Stop recording.
        else
        {
          m_state = State::STATUS_WAITING;
        }
      }
    }

    void verify(bool* const verifiyState, bool* const dataState, Sample const refValMax, Sample const refValMin, size_t const pos, size_t const package) const
    {
      if (*verifiyState && pos >= 0)
      {
        auto mainVal = m_timingsUs[pos];
        if (refValMin > mainVal || mainVal > refValMax)
        {
          //werte passen nicht
          *verifiyState = false;
        }
        if (m_state == State(size_t(State::STATUS_RECORDING_0) + package + 1) && *verifiyState == true)
        {
          *dataState = true;
        }
      }
    }

    void verification(size_t const package)
    {
      Sample refVal = m_timingsUs[m_dataEnd[package] - 1];
      Sample delta = refVal / 8 + refVal / 4; //+-37,5%
      Sample refValMin = refVal - delta;
      Sample refValMax = refVal + delta;
      size_t pos = m_dataEnd[package] - 1 - m_dataStart[package];

      switch (package)
      {
      case 1:
        verify(&m_pack0EqualPack1, &m_data1Ready, refValMax, refValMin, pos, package);
        break;
      case 2:
        verify(&m_pack0EqualPack2, &m_data1Ready, refValMax, refValMin, pos, package);
        verify(&m_pack1EqualPack2, &m_data2Ready, refValMax, refValMin, pos, package);
        if (m_state == State::STATUS_RECORDING_3 && m_data1Ready == false && m_data2Ready == false) {
          m_state = State::STATUS_WAITING;
        }
        break;
      case 3:
        if (!m_pack0EqualPack2)
          verify(&m_pack0EqualPack3, &m_data1Ready, refValMax, refValMin, pos, package);
        if (!m_pack1EqualPack2)
          verify(&m_pack1EqualPack3, &m_data2Ready, refValMax, refValMin, pos, package);
        if (m_state == State::STATUS_RECORDING_END && m_data1Ready == false && m_data2Ready == false) {
          m_state = State::STATUS_WAITING;
        }
        break;
      }
    }

  protected:
    constexpr static size_t c_maxRecordings = 512;
    constexpr static Sample c_minFooterUs = 3500;

    Sample m_timingsUs[c_maxRecordings];
    bool m_data1Ready = false;
    bool m_data2Ready = false;
    size_t m_dataStart[5] = {};
    size_t m_dataEnd[5] = {};
    enum class State {
      STATUS_WAITING,
      STATUS_RECORDING_0,
      STATUS_RECORDING_1,
      STATUS_RECORDING_2,
      STATUS_RECORDING_3,
      STATUS_RECORDING_END,
    } m_state = State::STATUS_WAITING;
    Sample m_footerUs = 0;
    bool m_pack0EqualPack1 = false;
    bool m_pack0EqualPack2 = false;
    bool m_pack0EqualPack3 = false;
    bool m_pack1EqualPack2 = false;
    bool m_pack1EqualPack3 = false;
  };
};
