#include <lib/remotecontrol/receiver_stm32.h>
#include <lib/common/math.h>
#include <lib/common/utils_stm32.h>

namespace RC
{
  namespace
  {
    class ReceiverImpl : public Receiver
    {
    public:
      ReceiverImpl(TIM_TypeDef* tim, const Pin::Def& pin)
        : m_timer(Rt::CreateHiresTimer(tim, Rt::HiresTimer::Callback::make<ReceiverImpl, &ReceiverImpl::periodic>(*this)))
        , m_pin(pin)
      {
        m_timer->start(1000000 / c_samplePeriodUs);
      }

      virtual bool receive(DurationUs& durationUs)
      {
        return m_samples.pop(durationUs);
      }

    protected:
      void periodic()
      {
        auto const lastState = m_filter.getState();
        auto currentDurationUs = m_currentDurationUs + c_samplePeriodUs;
    
        if (m_filter.next(m_pin.read()) || currentDurationUs >= c_maxDurationUs)
        {
          m_samples.push(lastState ? currentDurationUs : -currentDurationUs);
          currentDurationUs = 0;
        }

        m_currentDurationUs = currentDurationUs;
      }

    protected:
      constexpr static DurationUs c_samplePeriodUs = 10;
      constexpr static size_t c_samples = 200;
      constexpr static DurationUs c_maxDurationUs = 10000 - c_samplePeriodUs;

      //this filters out all pulses shorter than 9 samples
      constexpr static int c_filterShift = 3;
      constexpr static unsigned c_filterLowerPercent = 25;
      constexpr static unsigned c_filterUpperPercent = 75;

      std::unique_ptr<Rt::HiresTimer> m_timer;
      Pin::In const m_pin;
      math::BounceFilter<c_filterShift, c_filterLowerPercent, c_filterUpperPercent> m_filter;
      DurationUs m_currentDurationUs = 0;
      mstd::NonblockingQueue<DurationUs, c_samples> m_samples;
    };
  }
}

auto RC::CreateReceiverStm32(TIM_TypeDef* tim, const Pin::Def& pin) -> std::unique_ptr<Receiver>
{
  return std::make_unique<ReceiverImpl>(tim, pin);
}
