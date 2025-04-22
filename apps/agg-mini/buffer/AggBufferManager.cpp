#include "AggBufferManager.hpp"
#include "ns3/simulator.h"   // for Simulator::Schedule
#include "ns3/nstime.h"      // for Time
#include <ndn-cxx/name.hpp>
#include <functional>        // for std::function / lambda

namespace ns3 {
namespace ndn {

AggBufferManager::AggBufferManager(uint32_t capacity, Time timeout)
  : m_capacity(capacity)
  , m_timeout(timeout)
{
}

bool
AggBufferManager::CanInsert() const
{
  return m_map.size() < m_capacity;
}

bool
AggBufferManager::Insert(uint32_t seq,
                         const ::ndn::Name& parentName,
                         uint32_t expectedCount,
                         TimeoutCallback onTimeout)
{
  if (!CanInsert() || m_map.count(seq) > 0) {
    return false;
  }
  // Create one buffer entry
  auto buf = std::make_unique<AggBuffer>(expectedCount, parentName);

  // Schedule a lambda that simply calls your onTimeout(seq)
  auto timeoutFn = [onTimeout, seq]() {
    onTimeout(seq);
  };
  EventId ev = Simulator::Schedule(m_timeout, timeoutFn);
  buf->SetTimeoutEvent(ev);

  m_map.emplace(seq, std::move(buf));
  return true;
}

AggBuffer*
AggBufferManager::Get(uint32_t seq) const
{
  auto it = m_map.find(seq);
  return (it == m_map.end()) ? nullptr : it->second.get();
}

void
AggBufferManager::Remove(uint32_t seq)
{
  auto it = m_map.find(seq);
  if (it != m_map.end()) {
    it->second->CancelTimeoutEvent();
    m_map.erase(it);
  }
}

} // namespace ndn
} // namespace ns3