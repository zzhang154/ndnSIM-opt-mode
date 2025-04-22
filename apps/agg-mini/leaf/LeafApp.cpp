#include "LeafApp.hpp"
#include "ns3/log.h"
#include "ns3/names.h"      // <<< add
#include <string>           // <<< add

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

  std::string nodeName = Names::FindName(GetNode());
  NS_LOG_INFO("[" << nodeName << "] Started serving prefix " 
               << prefixValue.Get());
}

void
LeafApp::StopApplication()
{
  Producer::StopApplication();
  std::string nodeName = Names::FindName(GetNode());
  NS_LOG_INFO("[" << nodeName << "] Stopped");
}

void
LeafApp::OnInterest(shared_ptr<const Interest> interest)
{
  std::string nodeName = Names::FindName(GetNode());
  NS_LOG_INFO("[" << nodeName << "] Received Interest " 
               << interest->getName()
               << " nonce=" << interest->getNonce()
               << " lifetime=" << interest->getInterestLifetime().count() 
               << "ms");
  Producer::OnInterest(interest);
}

} // namespace ndn
} // namespace ns3