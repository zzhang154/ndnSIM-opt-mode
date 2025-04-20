// AggregatorApp.cpp
#include "AggregatorApp.hpp"
#include "ns3/log.h"
#include "ns3/string.h"  // For StringValue
#include "ns3/names.h"   // For Names functionality
#include "helper/ndn-fib-helper.hpp" // For FibHelper

NS_LOG_COMPONENT_DEFINE("ndn.AggregatorApp");

namespace ns3 {
namespace ndn {

// Moved inside namespace
NS_OBJECT_ENSURE_REGISTERED(AggregatorApp);

TypeId
AggregatorApp::GetTypeId()
{
  static TypeId tid =
    TypeId("ns3::ndn::AggregatorApp")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<AggregatorApp>()
      .AddAttribute("Prefix", "Prefix to serve upstream", StringValue("/agg"),
                    MakeNameAccessor(&AggregatorApp::m_downPrefix),
                    MakeNameChecker())
      .AddAttribute("ChildPrefix", "Prefix to request from leaf", StringValue("/leaf"),
                    MakeNameAccessor(&AggregatorApp::m_childPrefix),
                    MakeNameChecker());
  return tid;
}

AggregatorApp::AggregatorApp()
{
}

void
AggregatorApp::StartApplication()
{
  App::StartApplication();

  // add FIB route so Interests for m_downPrefix go to us
  FibHelper::AddRoute(GetNode(), m_downPrefix, m_face, 0);
  NS_LOG_INFO("[Node " << GetNode()->GetId() << "] AGG: Started application serving prefix " 
              << m_downPrefix << ", child prefix " << m_childPrefix);
}

void
AggregatorApp::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest);
  if (!m_active) return;

  std::string nodeInfo = "[Node " + std::to_string(GetNode()->GetId()) + "] AGG: ";
  NS_LOG_INFO(nodeInfo << "Received Interest " << interest->getName() 
              << " from nonce=" << interest->getNonce()
              << " lifetime=" << interest->getInterestLifetime().count() << "ms");

  // build child Interest name by replacing downPrefix with childPrefix
  Name childName = m_childPrefix;
  if (interest->getName().size() > m_downPrefix.size()) {
    auto suffix = interest->getName().getSubName(m_downPrefix.size());
    childName.append(suffix);
    NS_LOG_INFO(nodeInfo << "Interest suffix extraction: " << suffix);
  }
  
  auto childInterest = make_shared<Interest>();
  childInterest->setName(childName);
  childInterest->setCanBePrefix(false);
  childInterest->setNonce(interest->getNonce());
  childInterest->setInterestLifetime(interest->getInterestLifetime());

  NS_LOG_INFO(nodeInfo << "Forwarding Interest downstream: " << childName 
              << " (transformed from " << interest->getName() << ")");
  m_transmittedInterests(childInterest, this, m_face);
  m_appLink->onReceiveInterest(*childInterest);
}

void
AggregatorApp::OnData(shared_ptr<const Data> data)
{
  App::OnData(data);
  if (!m_active) return;

  std::string nodeInfo = "[Node " + std::to_string(GetNode()->GetId()) + "] AGG: ";
  NS_LOG_INFO(nodeInfo << "Received Data from downstream: " << data->getName());

  // repurpose Data for upstream consumer
  Name upName = m_downPrefix;
  if (data->getName().size() > m_childPrefix.size()) {
    auto suffix = data->getName().getSubName(m_childPrefix.size());
    upName.append(suffix);
    NS_LOG_INFO(nodeInfo << "Data suffix extraction: " << suffix);
  }
  
  auto upData = make_shared<Data>();
  upData->setName(upName);
  upData->setFreshnessPeriod(data->getFreshnessPeriod());
  upData->setContent(data->getContent());
  upData->setSignatureInfo(data->getSignatureInfo());
  
  // Fix for signature value type mismatch
  const auto& signatureValue = data->getSignatureValue();
  auto buffer = std::make_shared<::ndn::Buffer>(signatureValue.value(), 
                                            signatureValue.value_size());
  ::ndn::ConstBufferPtr constBuffer = buffer;
  upData->setSignatureValue(constBuffer);
  
  upData->wireEncode();

  NS_LOG_INFO(nodeInfo << "Forwarding Data upstream: " << upName 
              << " (transformed from " << data->getName() << ")"
              << " content size: " << upData->getContent().value_size() << " bytes"
              << " freshness: " << upData->getFreshnessPeriod().count() << "ms");
  m_transmittedDatas(upData, this, m_face);
  m_appLink->onReceiveData(*upData);
}

// Add this after StartApplication() implementation
void
AggregatorApp::StopApplication()
{
  App::StopApplication();
  
  NS_LOG_INFO("[Node " << GetNode()->GetId() << "] AGG: Stopped application");
}

} // namespace ndn
} // namespace ns3