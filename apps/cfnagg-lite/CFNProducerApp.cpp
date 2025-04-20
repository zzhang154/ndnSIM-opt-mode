#include "CFNProducerApp.hpp"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/integer.h"
#include <sstream>
#include "helper/ndn-stack-helper.hpp"  // Use the same import style as cfn-aggregator-app.cpp

NS_LOG_COMPONENT_DEFINE("ndn.CFNProducerApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(CFNProducerApp);

TypeId
CFNProducerApp::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::ndn::CFNProducerApp")
                          .SetGroupName("Ndn")
                          .SetParent<Producer>()
                          .AddConstructor<CFNProducerApp>()
                          .AddAttribute("PayloadValue",
                                        "Numeric payload to be sent in Data packet",
                                        IntegerValue(1),  // Changed from UintegerValue
                                        MakeIntegerAccessor(&CFNProducerApp::m_payloadValue),  // Changed
                                        MakeIntegerChecker<uint32_t>());  // Changed
  return tid;
}

CFNProducerApp::CFNProducerApp()
  : m_payloadValue(1)
{
  NS_LOG_FUNCTION(this);
}

CFNProducerApp::~CFNProducerApp()
{
}

void
CFNProducerApp::OnInterest(shared_ptr<const Interest> interest)
{
  NS_LOG_FUNCTION(this << interest);

  // Call base class for tracing purposes.
  Producer::OnInterest(interest);

  if (!m_active)
    return;

  // Create a Data packet with the same name as the incoming Interest.
  auto data = make_shared<Data>();
  data->setName(interest->getName());
  
  // Convert NS3 time to NDN time
  ::ndn::time::milliseconds freshness(1000); // 1 second
  data->setFreshnessPeriod(freshness);

  // Encode the numeric payload as a string.
  std::ostringstream oss;
  oss << m_payloadValue;
  std::string payloadStr = oss.str();
  
  // Create buffer using correct approach
  auto buffer = ::ndn::make_shared<::ndn::Buffer>(payloadStr.begin(), payloadStr.end());
  data->setContent(buffer);

  // Set up a dummy signature
  ::ndn::SignatureInfo signatureInfo(static_cast<::ndn::tlv::SignatureTypeValue>(255));
  data->setSignatureInfo(signatureInfo);

  // Wire encode the Data packet.
  data->wireEncode();

  NS_LOG_INFO("CFNProducerApp: Node " << GetNode()->GetId() << " replying with Data: "
              << data->getName() << " Content: " << payloadStr);

  // Transmit the Data packet.
  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
}

} // namespace ndn
} // namespace ns3