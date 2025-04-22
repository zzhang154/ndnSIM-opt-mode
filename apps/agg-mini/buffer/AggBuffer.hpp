#ifndef AGG_BUFFER_HPP
#define AGG_BUFFER_HPP

#include <ndn-cxx/name.hpp>
#include "ns3/event-id.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace ndn {

class AggBuffer {
public:
  AggBuffer(uint32_t expectedCount, const ::ndn::Name& parentInterest);
  ~AggBuffer();

  void AddValue(int value);
  void IncrementResponse();
  uint32_t GetExpectedCount() const;
  uint32_t GetReceivedCount() const;
  int      GetSum() const;
  bool     IsComplete() const;
  void     MarkReplied();
  bool     HasReplied() const;

  void     SetTimeoutEvent(EventId id);
  EventId  GetTimeoutEvent() const;
  void     CancelTimeoutEvent();

  const ::ndn::Name& GetParentInterestName() const;

private:
  uint32_t       m_expectedCount;
  uint32_t       m_receivedCount;
  int            m_sum;
  bool           m_replied;
  ::ndn::Name    m_parentInterestName;
  EventId        m_timeoutEvent;
};

} // namespace ndn
} // namespace ns3

#endif // AGG_BUFFER_HPP