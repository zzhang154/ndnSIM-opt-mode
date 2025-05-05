// RootApp.cpp
#include "RootApp.hpp"
// #include "ns3/log.h" // Replaced with iostream
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/type-id.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"  // For UintegerValue
#include "ns3/names.h"     // For Names::FindName
#include <iostream>        // For std::cout, std::endl
#include <string>          // For std::string

// NS_LOG_COMPONENT_DEFINE("ndn.RootApp"); // Replaced with std::cout

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
    std::cout << "RootApp: Constructor called." << std::endl;
}

// Optional: Add destructor if needed
RootApp::~RootApp()
{
    std::cout << "RootApp: Destructor called." << std::endl;
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
  std::cout << "RootApp: MaxSeq set to " << m_seqMax << std::endl;
}

void
RootApp::StartApplication() // Override to add logging
{
    Consumer::StartApplication(); // Call base class
    std::string nodeName = Names::FindName(GetNode());
    std::cout << "[" << nodeName << "] RootApp: StartApplication called. Sending Interests for prefix "
              << m_interestName << " up to seq=" << m_seqMax << " every " << m_interval << "s." << std::endl;
    // ScheduleNextPacket is called by Consumer::StartApplication
}

void
RootApp::StopApplication() // Override to add logging
{
    Consumer::StopApplication(); // Call base class
    std::string nodeName = Names::FindName(GetNode());
    std::cout << "[" << nodeName << "] RootApp: StopApplication called." << std::endl;
}


void
RootApp::ScheduleNextPacket()
{
  std::string nodeName = Names::FindName(GetNode());
  if (!m_active) {
      std::cout << "[" << nodeName << "] RootApp: ScheduleNextPacket called but app inactive." << std::endl;
      return;
  }

  // stop after m_seq reaches m_seqMax
  if (m_seq >= m_seqMax) {
    std::cout << "[" << nodeName << "] RootApp: Reached maximum sequence "
                << m_seqMax << ", stopping Interest generation." << std::endl;
    return;
  }

  // SendPacket() will be called by the base Consumer logic which is triggered by the timer
  // We just need to schedule the *next* one after the current one is sent.
  // The base Consumer::StartApplication likely calls SendPacket() initially.
  // Let's assume the base Consumer::ScheduleNextPacket handles the actual sending.
  // We override primarily to control the stopping condition and logging.

  // Consumer::ScheduleNextPacket(); // Call base class to handle actual scheduling and sending?
  // OR, if we need to manually schedule after sending:
  SendPacket(); // Send the current packet
  std::cout << "[" << nodeName << "] RootApp: Scheduling next packet (seq=" << (m_seq + 1) << "/" << m_seqMax << ") after " << m_interval << "s." << std::endl;
  Simulator::Schedule(Seconds(m_interval), &RootApp::ScheduleNextPacket, this); // Schedule the *next* call
}

// Add override for SendPacket to add logging
void
RootApp::SendPacket()
{
  std::string nodeName = Names::FindName(GetNode());
  if (!m_active) {
      std::cout << "[" << nodeName << "] RootApp: SendPacket called but app inactive." << std::endl;
      return;
  }

  std::cout << "[" << nodeName << "] RootApp: Sending Interest for "
              << m_interestName << "/seq=" << m_seq << std::endl;

  // Call parent class implementation to actually send the packet
  Consumer::SendPacket();
}

// Override OnData to add logging
void
RootApp::OnData(std::shared_ptr<const Data> data)
{
    std::string nodeName = Names::FindName(GetNode());
    std::cout << "[" << nodeName << "] RootApp: Received Data " << data->getName() << std::endl;

    // Extract and print the sum (assuming it's an int)
    int receivedSum = 0;
    const auto& content = data->getContent();
    if (content.hasValue() && content.value_size() >= sizeof(receivedSum)) {
        std::memcpy(&receivedSum, content.value(), sizeof(receivedSum));
        std::cout << "[" << nodeName << "] RootApp: Received aggregated sum = " << receivedSum << " for " << data->getName() << std::endl;
    } else {
        std::cout << "[" << nodeName << "] RootApp: WARNING - Received Data has missing or undersized content for " << data->getName() << std::endl;
    }

    // Call base class if it does anything important (like tracking RTT)
    Consumer::OnData(data);
}

// Override OnNack if needed
// void RootApp::OnNack(std::shared_ptr<const lp::Nack> nack) { ... }

// Override OnTimeout if needed
// void RootApp::OnTimeout(uint32_t sequenceNumber) { ... }


} // namespace ndn
} // namespace ns3