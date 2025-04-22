#include "AggBuffer.hpp"

namespace ns3 {
namespace ndn {

AggBuffer::AggBuffer(uint32_t expectedCount, const ::ndn::Name& parentInterestName)
  : m_expectedCount(expectedCount)
  , m_receivedCount(0)
  , m_sum(0)
  , m_replied(false)
  , m_parentInterestName(parentInterestName)
{
}

AggBuffer::~AggBuffer()
{
}

void
AggBuffer::AddValue(int value)
{
  m_sum += value;
}

void
AggBuffer::IncrementResponse()
{
  ++m_receivedCount;
}

uint32_t
AggBuffer::GetExpectedCount() const
{
  return m_expectedCount;
}

uint32_t
AggBuffer::GetReceivedCount() const
{
  return m_receivedCount;
}

int
AggBuffer::GetSum() const
{
  return m_sum;
}

bool
AggBuffer::IsComplete() const
{
  return m_receivedCount >= m_expectedCount;
}

void
AggBuffer::MarkReplied()
{
  m_replied = true;
}

bool
AggBuffer::HasReplied() const
{
  return m_replied;
}

void
AggBuffer::SetTimeoutEvent(EventId id)
{
  m_timeoutEvent = id;
}

EventId
AggBuffer::GetTimeoutEvent() const
{
  return m_timeoutEvent;
}

void
AggBuffer::CancelTimeoutEvent()
{
  if (m_timeoutEvent.IsRunning()) {
    Simulator::Cancel(m_timeoutEvent);
  }
}

const ::ndn::Name&
AggBuffer::GetParentInterestName() const
{
  return m_parentInterestName;
}

} // namespace ndn
} // namespace ns3