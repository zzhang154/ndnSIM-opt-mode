#include "LeafApp.hpp"
// #include "ns3/log.h" // Replaced with iostream
#include "ns3/names.h"      // <<< add
#include <string>           // <<< add
#include <iostream>         // <<< add for std::cout, std::endl

// NS_LOG_COMPONENT_DEFINE("ndn.LeafApp"); // Replaced with std::cout

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
  std::cout << "LeafApp: Constructor called." << std::endl;
  // nothing extra—Producer base handles OnInterest and Data replies
}

// Optional: Add destructor if needed
LeafApp::~LeafApp()
{
    std::cout << "LeafApp: Destructor called." << std::endl;
}

void
LeafApp::StartApplication()
{
  Producer::StartApplication(); // Call base class
  NameValue prefixValue;
  GetAttribute("Prefix", prefixValue);

  std::string nodeName = Names::FindName(GetNode());
  std::cout << "[" << nodeName << "] LeafApp: StartApplication called. Serving prefix "
               << prefixValue.Get() << std::endl;
}

void
LeafApp::StopApplication()
{
  Producer::StopApplication(); // Call base class
  std::string nodeName = Names::FindName(GetNode());
  std::cout << "[" << nodeName << "] LeafApp: StopApplication called." << std::endl;
}

void
LeafApp::OnInterest(shared_ptr<const Interest> interest)
{
  std::string nodeName = Names::FindName(GetNode());
  std::cout << "[" << nodeName << "] LeafApp: Received Interest "
               << interest->getName()
               << " nonce=" << interest->getNonce()
               << " lifetime=" << interest->getInterestLifetime().count()
               << "ms" << std::endl;

  // Call the base Producer's OnInterest which handles creating and sending the Data
  Producer::OnInterest(interest);
  std::cout << "[" << nodeName << "] LeafApp: Handled Interest via Producer base class for "
            << interest->getName() << std::endl;
}

// Optional: Override SendData if you need specific logging for Data sending
// void
// LeafApp::SendData(shared_ptr<Data> data, shared_ptr<pit::Entry> pitEntry)
// {
//   std::string nodeName = Names::FindName(GetNode());
//   std::cout << "[" << nodeName << "] LeafApp: Sending Data " << data->getName() << std::endl;
//   Producer::SendData(data, pitEntry); // Call base class implementation
// }


} // namespace ndn
} // namespace ns3