// filepath: /home/dd/ndnSIM-2.9-official/ndnSIM-2.9-official/src/ndnSIM/apps/cfnagg-lite/CFNRootApp.cpp
#include "CFNRootApp.hpp"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/integer.h" // Include for IntegerValue
#include "ns3/uinteger.h" // Include for UintegerValue if needed, or just use Integer

// ... other includes ...

NS_LOG_COMPONENT_DEFINE("ndn.CFNRootApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(CFNRootApp);

TypeId
CFNRootApp::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::ndn::CFNRootApp")
                          .SetGroupName("Ndn")
                          .SetParent<Consumer>() // Inherits attributes from Consumer
                          .AddConstructor<CFNRootApp>()
                          // Add the Rounds attribute
                          .AddAttribute("Rounds",
                                        "Number of aggregation rounds to initiate.",
                                        IntegerValue(1), // Default value
                                        MakeIntegerAccessor(&CFNRootApp::m_rounds),
                                        MakeIntegerChecker<int32_t>())
                          // REMOVE the StartSeq attribute definition here, as it's inherited
                          // .AddAttribute("StartSeq",
                          //               "Starting sequence number for Interests.",
                          //               IntegerValue(0), // Default value
                          //               MakeIntegerAccessor(&CFNRootApp::m_startSeq),
                          //               MakeIntegerChecker<int32_t>())
                          ; // Semicolon moved here
  return tid;
}

CFNRootApp::CFNRootApp()
  : m_rounds(1) // Initialize in constructor
  , m_currentRound(0)
  // m_startSeq is no longer needed here as a separate member,
  // the base class Consumer handles its own start sequence via the attribute.
{
  NS_LOG_FUNCTION(this);
  // SetSeqNoType(SeqNoType::SEQUENTIAL); // Keep this if needed
}

CFNRootApp::~CFNRootApp()
{
}

// Implement StartApplication to potentially use the inherited StartSeq
void
CFNRootApp::StartApplication()
{
  NS_LOG_FUNCTION(this);
  App::StartApplication(); // Call App's StartApplication, not Consumer's
  
  m_seq = 0; // Initialize sequence number
  m_currentRound = 0; // Reset round counter
  
  NS_LOG_INFO("Starting CFNRootApp. Initial Seq=" << m_seq << ". Total Rounds=" << m_rounds);
  
  // Schedule first packet after a short delay
  m_sendEvent = Simulator::Schedule(Seconds(0.1), &CFNRootApp::SendPacket, this);
}

void
CFNRootApp::SendPacket()
{
  NS_LOG_FUNCTION(this);
  
  if (!m_active) {
    NS_LOG_INFO("CFNRootApp not active, not sending packet");
    return;
  }
  
  // Check if we've reached max rounds
  if (m_currentRound >= m_rounds) {
    NS_LOG_INFO("Maximum rounds (" << m_rounds << ") reached. Stopping.");
    return;
  }
  
  // Create the interest name with sequence number
  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(m_seq);
  
  // Create the interest packet
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  interest->setCanBePrefix(true);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);
  
  NS_LOG_INFO("CFNRootApp: Sending Interest " << interest->getName() 
              << " (Round=" << m_currentRound+1 << "/" << m_rounds 
              << ", Seq=" << m_seq << ")");
  
  // Track metrics for Consumer's statistics if needed
  WillSendOutInterest(m_seq);
  
  // Transmit the interest
  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
  
  // Increment sequence number for next time
  m_seq++;
  
  // Update rounds
  m_currentRound++;
  
  // Schedule next packet
  ScheduleNextPacket();
}

void
CFNRootApp::ScheduleNextPacket()
{
  NS_LOG_FUNCTION(this);
  
  // Only schedule if we haven't reached max rounds
  if (m_currentRound >= m_rounds) {
    NS_LOG_INFO("Maximum rounds (" << m_rounds << ") reached. Not scheduling more packets.");
    return;
  }
  
  // Schedule with a fixed delay
  Time delay = Seconds(1.0);
  NS_LOG_DEBUG("Scheduling next Interest for round " << (m_currentRound + 1) << " after " << delay.GetSeconds() << "s");
  
  if (!m_sendEvent.IsRunning()) {
    m_sendEvent = Simulator::Schedule(delay, &CFNRootApp::SendPacket, this);
  } else {
    NS_LOG_WARN("Send event already running, not scheduling another one");
  }
}

void
CFNRootApp::OnData(shared_ptr<const Data> data)
{
  // Process the Data packet via the base Consumer functionality if applicable.
  Consumer::OnData(data);

  // Use m_seq for context
  uint32_t received_seq = data->getName().at(data->getName().size() - 1).toSequenceNumber();
  std::string content(reinterpret_cast<const char*>(data->getContent().value()),
                      data->getContent().value_size());
  NS_LOG_INFO("CFNRootApp: Received aggregated Data for seq=" << received_seq << " with sum=" << content);
}

} // namespace ndn
} // namespace ns3
