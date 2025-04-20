#include "LeafApp.hpp"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("ndn.LeafApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(LeafApp);

TypeId
LeafApp::GetTypeId()
{
  static TypeId tid =
    TypeId("ns3::ndn::LeafApp")
      .SetGroupName("Ndn")
      .SetParent<Producer>()
      .AddConstructor<LeafApp>();
  return tid;
}

LeafApp::LeafApp()
{
  // nothing extra—Producer base handles OnInterest and Data replies
}

void
LeafApp::StartApplication()
{
  Producer::StartApplication();
  NameValue prefixValue;
  GetAttribute("Prefix", prefixValue);
  NS_LOG_INFO("[Node " << GetNode()->GetId() << "] LEAF: Started application serving prefix " << prefixValue.Get());
}

void
LeafApp::StopApplication()
{
  Producer::StopApplication();
  NS_LOG_INFO("[Node " << GetNode()->GetId() << "] LEAF: Stopped application");
}

void
LeafApp::OnInterest(shared_ptr<const Interest> interest)
{
  NS_LOG_INFO("[Node " << GetNode()->GetId() << "] LEAF: Received Interest " << interest->getName()
              << " nonce=" << interest->getNonce()
              << " lifetime=" << interest->getInterestLifetime().count() << "ms");
  Producer::OnInterest(interest);
}

} // namespace ndn
} // namespace ns3