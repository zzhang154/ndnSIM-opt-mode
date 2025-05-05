#include "AggBufferManager.hpp"
#include "ns3/simulator.h"   // for Simulator::Schedule
#include "ns3/nstime.h"      // for Time
#include <ndn-cxx/name.hpp>
#include <functional>        // for std::function / lambda
#include <iostream>          // for std::cout, std::endl

namespace ns3 {
namespace ndn {

AggBufferManager::AggBufferManager(uint32_t capacity, Time timeout)
  : m_capacity(capacity)
  , m_timeout(timeout)
{
  std::cout << "AggBufferManager: Initialized with capacity=" << m_capacity
            << ", timeout=" << m_timeout.ToDouble(Time::S) << "s" << std::endl;
}

bool
AggBufferManager::CanInsert() const
{
  bool can = m_map.size() < m_capacity;
  if (!can) {
      std::cout << "AggBufferManager: Cannot insert, buffer full (size=" << m_map.size()
                << ", capacity=" << m_capacity << ")" << std::endl;
  }
  return can;
}

bool
AggBufferManager::Insert(uint32_t seq,
                         const ::ndn::Name& parentName,
                         uint32_t expectedCount,
                         TimeoutCallback onTimeout)
{
  if (m_map.count(seq) > 0) {
    std::cout << "AggBufferManager: Cannot insert, duplicate seq=" << seq << std::endl;
    return false;
  }
  if (!CanInsert()) {
    // CanInsert already printed the reason
    return false;
  }

  // Create one buffer entry
  auto buf = std::make_unique<AggBuffer>(expectedCount, parentName);
  std::cout << "AggBufferManager: Inserting buffer for seq=" << seq
            << ", parent=" << parentName << ", expecting=" << expectedCount << std::endl;

  // Schedule a lambda that simply calls your onTimeout(seq)
  auto timeoutFn = [onTimeout, seq]() {
    std::cout << "AggBufferManager: Timeout scheduled function executing for seq=" << seq << std::endl;
    onTimeout(seq);
  };
  EventId ev = Simulator::Schedule(m_timeout, timeoutFn);
  buf->SetTimeoutEvent(ev);
  std::cout << "AggBufferManager: Scheduled timeout event " << ev.GetUid()
            << " for seq=" << seq << " in " << m_timeout.ToDouble(Time::S) << "s" << std::endl;

  m_map.emplace(seq, std::move(buf));
  return true;
}

AggBuffer*
AggBufferManager::Get(uint32_t seq) const
{
  auto it = m_map.find(seq);
  AggBuffer* bufPtr = (it == m_map.end()) ? nullptr : it->second.get();
  if (!bufPtr) {
      std::cout << "AggBufferManager: Get failed for seq=" << seq << ", buffer not found." << std::endl;
  } else {
      std::cout << "AggBufferManager: Get successful for seq=" << seq << std::endl;
  }
  return bufPtr;
}

void
AggBufferManager::Remove(uint32_t seq)
{
  auto it = m_map.find(seq);
  if (it != m_map.end()) {
    std::cout << "AggBufferManager: Removing buffer for seq=" << seq << std::endl;
    it->second->CancelTimeoutEvent(); // Cancel associated timeout event
    m_map.erase(it);
  } else {
    std::cout << "AggBufferManager: Remove called for non-existent seq=" << seq << std::endl;
  }
}

} // namespace ndn
} // namespace ns3