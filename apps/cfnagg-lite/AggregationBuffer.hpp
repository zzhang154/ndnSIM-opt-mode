#ifndef AGGREGATION_BUFFER_HPP
#define AGGREGATION_BUFFER_HPP

#include <ndn-cxx/name.hpp>
#include "ns3/event-id.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace ndn {

using ::ndn::Name;

class AggregationBuffer {
public:
  AggregationBuffer(uint32_t expectedCount, const Name& parentInterestName);
  ~AggregationBuffer();

  // Add a numeric value from a child Data packet.
  void AddValue(int value);

  // Increment the counter of received responses.
  void IncrementResponse();

  // Return the expected number of responses.
  uint32_t GetExpectedCount() const;

  // Return the number of responses received so far.
  uint32_t GetReceivedCount() const;

  // Return the running sum.
  int GetSum() const;

  // Returns true if responses received meet or exceed expected count.
  bool IsComplete() const;

  // Mark that an aggregated Data packet has been sent.
  void MarkReplied();

  // Check if a reply has been sent.
  bool HasReplied() const;
  
  // Manage the timeout event associated with this aggregation round.
  void SetTimeoutEvent(ns3::EventId id);
  ns3::EventId GetTimeoutEvent() const;
  void CancelTimeoutEvent();

  // Accessor for the parent Interest name.
  const Name& GetParentInterestName() const;

private:
  uint32_t m_expectedCount;
  uint32_t m_receivedCount;
  int m_sum;
  bool m_replied;
  Name m_parentInterestName;
  ns3::EventId m_timeoutEvent;
};

} // namespace ndn
} // namespace ns3

#endif // AGGREGATION_BUFFER_HPP
