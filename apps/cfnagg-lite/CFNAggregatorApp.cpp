#include "CFNAggregatorApp.hpp"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include <sstream>
#include <cstdlib>

NS_LOG_COMPONENT_DEFINE("ndn.CFNAggregatorApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(CFNAggregatorApp);

TypeId
CFNAggregatorApp::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::ndn::CFNAggregatorApp")
                          .SetGroupName("Ndn")
                          .SetParent<App>()
                          .AddConstructor<CFNAggregatorApp>()
                          .AddAttribute("PartialTimeout",
                                        "Timeout for waiting child responses before partial aggregation",
                                        TimeValue(MilliSeconds(20)),
                                        MakeTimeAccessor(&CFNAggregatorApp::m_partialTimeout),
                                        MakeTimeChecker())
                          // Add the Prefix attribute definition
                          .AddAttribute("Prefix",
                            "Prefix that the aggregator will serve to its parent.",
                            NameValue("/"), // Default value
                            MakeNameAccessor(&CFNAggregatorApp::m_prefix),
                            MakeNameChecker());
  return tid;
}

CFNAggregatorApp::CFNAggregatorApp()
  : m_partialTimeout(MilliSeconds(20))
{
  NS_LOG_FUNCTION(this);
}

CFNAggregatorApp::~CFNAggregatorApp()
{
}

void
CFNAggregatorApp::StartApplication()
{
  App::StartApplication(); // creates m_face

  // Schedule re-insertion after global routing
  Simulator::Schedule(Seconds(0.02), [this] {
    FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
    NS_LOG_INFO("CFNAggregatorApp: Re-added FIB route for " << m_prefix << " on faceId=" << m_face->getId());
  });
}

void
CFNAggregatorApp::StopApplication()
{
  NS_LOG_FUNCTION(this);
  // Cancel any pending timeout events.
  for (auto& entry : m_aggBufferMap) {
    entry.second.CancelTimeoutEvent();
  }
  m_aggBufferMap.clear();
  App::StopApplication();
}

uint32_t
CFNAggregatorApp::ExtractSequenceNumber(const Name& name)
{
  // Assumes that the sequence number is the last component of the name.
  return name.at(name.size() - 1).toSequenceNumber();
}

void
CFNAggregatorApp::OnInterest(shared_ptr<const Interest> interest)
{
  NS_LOG_FUNCTION(this << interest);

  // This Interest is from the parent.
  uint32_t seq = ExtractSequenceNumber(interest->getName());
  NS_LOG_INFO("CFNAggregatorApp: Received Interest " << interest->getName() << " (seq=" << seq << ")");

  // Create an aggregation buffer entry for this sequence.
  uint32_t expected = m_childrenPrefixes.size();
  
  // Use emplace to properly construct the AggregationBuffer
  auto result = m_aggBufferMap.emplace(seq, AggregationBuffer(expected, interest->getName()));
  auto& buffer = result.first->second;
  
  // Schedule a timeout for partial aggregation.
  EventId timeoutEvent = Simulator::Schedule(m_partialTimeout, &CFNAggregatorApp::AggregationTimeout, this, seq);
  buffer.SetTimeoutEvent(timeoutEvent);

  // Forward the Interest to all child nodes.
  ForwardInterestToChildren(seq);
}

void
CFNAggregatorApp::ForwardInterestToChildren(uint32_t seq)
{
  NS_LOG_FUNCTION(this << seq);
  for (const Name& childPrefix : m_childrenPrefixes) {
    Name childInterestName = childPrefix;
    childInterestName.appendSequenceNumber(seq);
    auto interest = make_shared<Interest>();
    interest->setName(childInterestName);
    // Use the current simulation time to generate a nonce.
    interest->setNonce(static_cast<uint32_t>(Simulator::Now().GetNanoSeconds()));
    
    // Convert ns3::Time to ndn::time::milliseconds
    ::ndn::time::milliseconds lifetime(m_partialTimeout.GetMilliSeconds() + 5);
    interest->setInterestLifetime(lifetime);
    
    interest->setCanBePrefix(false);
    NS_LOG_INFO("CFNAggregatorApp: Forwarding Interest " << interest->getName()
                 << " to child " << childPrefix.toUri());
    m_transmittedInterests(interest, this, m_face);
    m_appLink->onReceiveInterest(*interest);
  }
}

void
CFNAggregatorApp::AggregationTimeout(uint32_t seq)
{
  NS_LOG_FUNCTION(this << seq);
  auto it = m_aggBufferMap.find(seq);
  if (it == m_aggBufferMap.end())
    return;
  AggregationBuffer& buffer = it->second;
  
  if (buffer.HasReplied())
    return;
    
  NS_LOG_INFO("CFNAggregatorApp: Timeout for seq=" << seq << " with partial aggregation sum=" << buffer.GetSum()
              << " (" << buffer.GetReceivedCount() << "/" << buffer.GetExpectedCount() << " responses)");

  // Produce a Data packet with the (partial) aggregated result.
  auto data = make_shared<Data>();
  data->setName(buffer.GetParentInterestName());
  
  // Convert ns3::Time to ndn::time::milliseconds
  ::ndn::time::milliseconds freshness(1000); // 1 second
  data->setFreshnessPeriod(freshness);
  
  std::ostringstream oss;
  oss << buffer.GetSum();
  std::string sumStr = oss.str();
  
  // Use proper ndn buffer creation
  auto buffer_content = ::ndn::make_shared<::ndn::Buffer>(sumStr.begin(), sumStr.end());
  data->setContent(buffer_content);

  // Properly create signature info with namespace
  ::ndn::SignatureInfo signatureInfo(static_cast<::ndn::tlv::SignatureTypeValue>(255));
  data->setSignatureInfo(signatureInfo);
  
  data->wireEncode();

  NS_LOG_INFO("CFNAggregatorApp: Sending (partial) aggregated Data for seq=" << seq << " with sum=" << sumStr);
  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);

  buffer.MarkReplied();
  m_aggBufferMap.erase(it);
}

void
CFNAggregatorApp::OnData(shared_ptr<const Data> data)
{
  NS_LOG_FUNCTION(this << data);

  // Assume Data from child nodes is named as: childPrefix/seq.
  uint32_t seq = ExtractSequenceNumber(data->getName());
  NS_LOG_INFO("CFNAggregatorApp: Received child Data " << data->getName() << " for seq=" << seq);

  auto it = m_aggBufferMap.find(seq);
  if (it == m_aggBufferMap.end()) {
    NS_LOG_WARN("CFNAggregatorApp: No aggregation buffer for seq=" << seq << ", ignoring Data");
    return;
  }
  AggregationBuffer& buffer = it->second;

  if (buffer.HasReplied()) {
    NS_LOG_WARN("CFNAggregatorApp: Already replied for seq=" << seq << ", ignoring late Data");
    return;
  }

  // Extract the numeric payload from the Data.
  std::string content(reinterpret_cast<const char*>(data->getContent().value()),
                      data->getContent().value_size());
  int value = std::stoi(content);
  
  buffer.AddValue(value);
  buffer.IncrementResponse();

  NS_LOG_INFO("CFNAggregatorApp: Updated aggregation for seq=" << seq << ": sum=" << buffer.GetSum()
              << " (" << buffer.GetReceivedCount() << "/" << buffer.GetExpectedCount() << ")");

  // If all child responses have been received, cancel the timeout and reply.
  if (buffer.IsComplete()) {
    buffer.CancelTimeoutEvent();

    auto aggData = make_shared<Data>();
    aggData->setName(buffer.GetParentInterestName());
    
    // Convert to proper ndn time format
    ::ndn::time::milliseconds freshness(1000); // 1 second
    aggData->setFreshnessPeriod(freshness);
    
    std::ostringstream oss;
    oss << buffer.GetSum();
    std::string sumStr = oss.str();
    
    // Use proper ndn buffer creation
    auto buffer_content = ::ndn::make_shared<::ndn::Buffer>(sumStr.begin(), sumStr.end());
    aggData->setContent(buffer_content);

    // Properly create signature info with namespace
    ::ndn::SignatureInfo signatureInfo(static_cast<::ndn::tlv::SignatureTypeValue>(255));
    aggData->setSignatureInfo(signatureInfo);
    
    aggData->wireEncode();

    NS_LOG_INFO("CFNAggregatorApp: Sending final aggregated Data for seq=" << seq << " with sum=" << sumStr);
    m_transmittedDatas(aggData, this, m_face);
    m_appLink->onReceiveData(*aggData);

    buffer.MarkReplied();
    m_aggBufferMap.erase(it);
  }
}

void
CFNAggregatorApp::AddChildPrefix(const Name& prefix)
{
  m_childrenPrefixes.push_back(prefix);
}

} // namespace ndn
} // namespace ns3