// RootApp.cpp
#include "RootApp.hpp"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/type-id.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"  // For UintegerValue

NS_LOG_COMPONENT_DEFINE("ndn.RootApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(RootApp);

TypeId
RootApp::GetTypeId()
{
  static TypeId tid =
    TypeId("ns3::ndn::RootApp")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<RootApp>()
      .AddAttribute("Interval", "Time between Interests (s)", 
                    DoubleValue(1.0),
                    MakeDoubleAccessor(&RootApp::m_interval),
                    MakeDoubleChecker<double>())
      .AddAttribute("MaxSeq", "Maximum number of sequences to send",
                    UintegerValue(100),  
                    MakeUintegerAccessor(&RootApp::GetMaxSeq,  // Use RootApp methods
                                        &RootApp::SetMaxSeq),  // Not Consumer methods 
                    MakeUintegerChecker<uint32_t>());
  return tid;
}

RootApp::RootApp()
 : m_interval(1.0)
{
}

// Implement the accessor methods declared in RootApp.hpp
uint32_t 
RootApp::GetMaxSeq() const
{
  return m_seqMax; // Access the protected member from Consumer
}

void 
RootApp::SetMaxSeq(uint32_t maxSeq)
{
  m_seqMax = maxSeq; // Set the protected member from Consumer
}

void
RootApp::ScheduleNextPacket()
{
  if (!m_active) return;

  // stop after m_seq reaches m_seqMax
  if (m_seq >= m_seqMax) {
    NS_LOG_INFO("[Node " << GetNode()->GetId() << "] ROOT: Reached maximum sequence " 
                << m_seqMax << ", stopping Interest generation");
    return;
  }

  SendPacket();
  NS_LOG_INFO("[Node " << GetNode()->GetId() << "] ROOT: Scheduled next packet, sequence " 
              << m_seq << "/" << m_seqMax << " after " << m_interval << "s");
  Simulator::Schedule(Seconds(m_interval), &RootApp::ScheduleNextPacket, this);
}

// Add override for SendPacket to add logging
void
RootApp::SendPacket()
{
  if (!m_active) return;

  NS_LOG_INFO("[Node " << GetNode()->GetId() << "] ROOT: Sending Interest for " 
              << m_interestName << "/seq=" << m_seq);

  // Call parent class implementation
  Consumer::SendPacket();
}

} // namespace ndn
} // namespace ns3