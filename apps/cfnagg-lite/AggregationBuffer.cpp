#include "AggregationBuffer.hpp"

namespace ns3 {
namespace ndn {

AggregationBuffer::AggregationBuffer(uint32_t expectedCount, const Name& parentInterestName)
  : m_expectedCount(expectedCount)
  , m_receivedCount(0)
  , m_sum(0)
  , m_replied(false)
  , m_parentInterestName(parentInterestName)
{
}

AggregationBuffer::~AggregationBuffer()
{
}

void
AggregationBuffer::AddValue(int value)
{
  m_sum += value;
}

void
AggregationBuffer::IncrementResponse()
{
  ++m_receivedCount;
}

uint32_t
AggregationBuffer::GetExpectedCount() const
{
  return m_expectedCount;
}

uint32_t
AggregationBuffer::GetReceivedCount() const
{
  return m_receivedCount;
}

int
AggregationBuffer::GetSum() const
{
  return m_sum;
}

bool
AggregationBuffer::IsComplete() const
{
  return m_receivedCount >= m_expectedCount;
}

void
AggregationBuffer::MarkReplied()
{
  m_replied = true;
}

bool
AggregationBuffer::HasReplied() const
{
  return m_replied;
}

void
AggregationBuffer::SetTimeoutEvent(ns3::EventId id)
{
  m_timeoutEvent = id;
}

ns3::EventId
AggregationBuffer::GetTimeoutEvent() const
{
  return m_timeoutEvent;
}

void
AggregationBuffer::CancelTimeoutEvent()
{
  if (m_timeoutEvent.IsRunning())
    ns3::Simulator::Cancel(m_timeoutEvent);
}

const Name&
AggregationBuffer::GetParentInterestName() const
{
  return m_parentInterestName;
}

} // namespace ndn
} // namespace ns3
