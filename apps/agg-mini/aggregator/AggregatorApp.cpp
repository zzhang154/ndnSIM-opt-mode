#include "AggregatorApp.hpp"
#include "ns3/object.h"                // for NS_OBJECT_ENSURE_REGISTERED
#include "helper/ndn-fib-helper.hpp"
// #include "ns3/log.h" // Replaced with iostream
#include "ns3/names.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"              // <<< add
#include "ns3/nstime.h"
#include <sstream>
#include <iostream> // <<< add for std::cout, std::endl
#include <string>   // <<< add for std::string

#include <ndn-cxx/util/time.hpp>        // for ndn::time::milliseconds
#include <ndn-cxx/encoding/buffer.hpp>    // for ndn::Buffer
#include <ndn-cxx/signature-info.hpp>     // for ndn::SignatureInfo
#include <ndn-cxx/encoding/estimator.hpp> // for ndn::EncodingEstimator
#include <ndn-cxx/encoding/buffer.hpp>    // for ndn::EncodingBuffer

// NS_LOG_COMPONENT_DEFINE("ndn.AggregatorApp"); // Replaced with std::cout

namespace ns3 {
namespace ndn {

// <<–– MUST BE INSIDE this namespace ––>>
NS_OBJECT_ENSURE_REGISTERED(AggregatorApp);

TypeId
AggregatorApp::GetTypeId()
{
  static TypeId tid =
    TypeId("ns3::ndn::AggregatorApp")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<AggregatorApp>()
      .AddAttribute("Prefix", "Upstream prefix",
                    StringValue("/app/agg"),
                    MakeNameAccessor(&AggregatorApp::m_downPrefix),
                    MakeNameChecker())
      .AddAttribute("ChildPrefixes", "Space-separated leaf prefixes",
                    StringValue(""),
                    MakeStringAccessor(&AggregatorApp::m_childPrefixesRaw),
                    MakeStringChecker())
      .AddAttribute("BufferCapacity", "Max in-flight sequences",
                    UintegerValue(100),
                    MakeUintegerAccessor(&AggregatorApp::m_bufferCapacity),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("StragglerTimeout", "Wait for missing children",
                    TimeValue(Seconds(1.0)),
                    MakeTimeAccessor(&AggregatorApp::m_stragglerTimeout),
                    MakeTimeChecker());
  return tid;
}

AggregatorApp::AggregatorApp()
  : m_bufferCapacity(0)
  , m_stragglerTimeout(Seconds(0))
  , m_bufferMgr(0, Seconds(0)) // Will be re-initialized in StartApplication
{
    std::cout << "AggregatorApp: Constructor called." << std::endl;
}

// Optional: Add destructor if needed for cleanup logging
AggregatorApp::~AggregatorApp()
{
    // Note: Accessing node name here might be unsafe if node is already destroyed
    std::cout << "AggregatorApp: Destructor called." << std::endl;
}


void
AggregatorApp::StartApplication()
{
  App::StartApplication();  // This should initialize m_face and m_appLink
  std::string nodeName = Names::FindName(GetNode());
  std::cout << "[" << nodeName << "] AggregatorApp: StartApplication called." << std::endl;

  // Verify that initialization worked
  if (!m_face || !m_appLink) {
    std::cerr << "[" << nodeName << "] AggregatorApp: ERROR - Face or AppLink not initialized properly" << std::endl;
    return;
  } else {
    std::cout << "[" << nodeName << "] AggregatorApp: Face and AppLink initialized." << std::endl;
  }


  FibHelper::AddRoute(GetNode(), m_downPrefix, m_face, 0);
  std::cout << "[" << nodeName << "] AggregatorApp: Added FIB route for " << m_downPrefix << " via AppFace." << std::endl;

  m_childPrefixes.clear(); // Clear in case StartApplication is called multiple times
  std::istringstream iss(m_childPrefixesRaw);
  std::string token;
  while (iss >> token) {
    try {
        m_childPrefixes.emplace_back(token);
    } catch (const std::exception& e) {
        std::cerr << "[" << nodeName << "] AggregatorApp: ERROR - Invalid child prefix format: '" << token << "'. Error: " << e.what() << std::endl;
    }
  }

  // init manager with real attrs read from config
  m_bufferMgr = AggBufferManager(m_bufferCapacity, m_stragglerTimeout);
  std::cout << "[" << nodeName << "] AggregatorApp: BufferManager re-initialized with capacity="
            << m_bufferCapacity << ", timeout=" << m_stragglerTimeout.ToDouble(Time::S) << "s" << std::endl;

  std::cout << "[" << nodeName << "] AggregatorApp: Serving " << m_downPrefix
               << " -> children={" << m_childPrefixesRaw << "}" << std::endl;
}

void
AggregatorApp::OnInterest(std::shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // Call base class first
  std::string nodeName = Names::FindName(GetNode());
  if (!m_active) {
      std::cout << "[" << nodeName << "] AggregatorApp: OnInterest called but app inactive." << std::endl;
      return;
  }

  uint32_t seq = interest->getName().get(-1).toSequenceNumber();
  std::cout << "[" << nodeName << "] AggregatorApp: RX Interest " << interest->getName() << std::endl;

  if (!m_bufferMgr.Insert(seq,
                          interest->getName(),
                          m_childPrefixes.size(),
                          MakeCallback(&AggregatorApp::OnStragglerTimeout, this)))
  {
    // Buffer manager already printed the reason (full or duplicate)
    // NS_LOG_WARN("[" << nodeName << "] buffer full or dup seq=" << seq); // Replaced
    return;
  }

  // fan‑out to children
  for (auto const& childPrefix : m_childPrefixes) {
    // build full child name = childPrefix + suffix after m_downPrefix
    Name childName = childPrefix;
    if (interest->getName().size() > m_downPrefix.size()) {
      auto suffix = interest->getName().getSubName(m_downPrefix.size());
      childName.append(suffix);
    }
    auto childInterest = std::make_shared<Interest>();
    childInterest->setName(childName);
    childInterest->setCanBePrefix(false); // Assuming leaves expect exact match
    childInterest->setNonce(interest->getNonce()); // Reuse nonce for potential loop detection
    childInterest->setInterestLifetime(interest->getInterestLifetime()); // Propagate lifetime

    std::cout << "[" << nodeName << "] AggregatorApp: FWD Interest to " << childName << std::endl;

    // Send the interest using the AppLink/Face
    if (m_appLink) {
      m_appLink->onReceiveInterest(*childInterest); // Send Interest downstream
    } else {
      std::cerr << "[" << nodeName << "] AggregatorApp: ERROR - m_appLink is null when sending interest to " << childName << std::endl;
    }
  }
}

void
AggregatorApp::OnData(std::shared_ptr<const Data> data)
{
  App::OnData(data); // Call base class first
  std::string nodeName = Names::FindName(GetNode());
   if (!m_active) {
      std::cout << "[" << nodeName << "] AggregatorApp: OnData called but app inactive." << std::endl;
      return;
  }

  uint32_t seq = data->getName().get(-1).toSequenceNumber();
  std::cout << "[" << nodeName << "] AggregatorApp: RX Data " << data->getName() << std::endl;

  auto buf = m_bufferMgr.Get(seq);
  if (!buf) {
    // Buffer manager already printed the reason
    // NS_LOG_WARN("[" << nodeName << "] no buffer for seq=" << seq); // Replaced
    return;
  }

  // extract integer payload safely
  int value = 0;
  {
    const ::ndn::Block& block = data->getContent();
    if (block.hasValue()) {
        size_t copyLen = std::min(block.value_size(), sizeof(value));
        std::memcpy(&value, block.value(), copyLen);
        if (copyLen < sizeof(value)) {
             std::cout << "[" << nodeName << "] AggregatorApp: WARNING - Received Data payload smaller than expected size for seq=" << seq
                       << ". Size=" << block.value_size() << ", expected=" << sizeof(value) << ". Using partial value." << std::endl;
        }
    } else {
         std::cout << "[" << nodeName << "] AggregatorApp: WARNING - Received Data has no content payload for seq=" << seq << ". Using value 0." << std::endl;
    }
  }

  buf->AddValue(value);
  buf->IncrementResponse();
  std::cout << "[" << nodeName << "] AggregatorApp: Added value=" << value << " to buffer seq=" << seq
            << ". Count=" << buf->GetReceivedCount() << "/" << buf->GetExpectedCount() << std::endl;


  if (buf->IsComplete()) {
    std::cout << "[" << nodeName << "] AggregatorApp: Buffer complete for seq=" << seq << ". Sending aggregated data up." << std::endl;
    Name upName = buf->GetParentInterestName();
    auto aggData = std::make_shared<Data>();
    aggData->setName(upName);
    aggData->setFreshnessPeriod(ndn::time::milliseconds(0)); // Data is immediately stale

    // marshal the sum into a local so & is valid
    int sum = buf->GetSum();
    std::cout << "[" << nodeName << "] AggregatorApp: Aggregated sum=" << sum << " for seq=" << seq << std::endl;
    // Copy the sum directly into Data’s content (Data::setContent makes an internal copy)
    aggData->setContent(reinterpret_cast<const uint8_t*>(&sum), sizeof(sum));

    ns3::ndn::common::SetDummySignature(aggData); // Using common utils is fine
    aggData->wireEncode(); // Ensure wire format is ready

    std::cout << "[" << nodeName << "] AggregatorApp: AGG send up " << upName << std::endl;

    if (m_appLink) {
      m_appLink->onReceiveData(*aggData); // Correct way to send Data up
    } else {
      std::cerr << "[" << nodeName << "] AggregatorApp: ERROR - m_appLink is null when sending data up for " << upName << std::endl;
    }

    m_bufferMgr.Remove(seq); // Clean up buffer after sending
  }
}

void
AggregatorApp::OnStragglerTimeout(uint32_t seq)
{
  std::string nodeName = Names::FindName(GetNode());
  std::cout << "[" << nodeName << "] AggregatorApp: Straggler timeout triggered for seq=" << seq << std::endl;

  auto buf = m_bufferMgr.Get(seq);
  // Check if buffer exists AND if we haven't already replied (due to completion)
  if (!buf || buf->HasReplied()) {
      std::cout << "[" << nodeName << "] AggregatorApp: Timeout for seq=" << seq
                   << " - buffer gone or already replied. Ignoring timeout." << std::endl;
      // If buffer is gone but we haven't removed it yet (e.g., race with completion),
      // ensure it gets removed if it still exists in the map.
      if (!buf) m_bufferMgr.Remove(seq);
      return;
  }

  Name upName = buf->GetParentInterestName();
  auto aggData = std::make_shared<Data>();
  aggData->setName(upName);
  aggData->setFreshnessPeriod(ndn::time::milliseconds(0));

  // local sum for addressability
  int sum = buf->GetSum();
  std::cout << "[" << nodeName << "] AggregatorApp: Timeout - Aggregated sum=" << sum << " for seq=" << seq << std::endl;

  // Copy the sum directly into Data’s content
  aggData->setContent(reinterpret_cast<const uint8_t*>(&sum), sizeof(sum));

  ns3::ndn::common::SetDummySignature(aggData);
  aggData->wireEncode(); // Ensure wire format is ready

  std::cout << "[" << nodeName << "] AggregatorApp: Timeout-AGG send up " << upName
               << " (Received " << buf->GetReceivedCount() << "/" << buf->GetExpectedCount() << ")" << std::endl;

  // Send Data up via AppLink
  if (m_appLink) {
     m_appLink->onReceiveData(*aggData);
  } else {
     std::cerr << "[" << nodeName << "] AggregatorApp: ERROR - m_appLink is null during timeout handling for " << upName << std::endl;
  }

  buf->MarkReplied(); // Mark as replied *before* removing to prevent race conditions
  m_bufferMgr.Remove(seq); // Remove after sending
}

void
AggregatorApp::StopApplication()
{
  App::StopApplication(); // Call base class
  std::string nodeName = Names::FindName(GetNode());
  std::cout << "[" << nodeName << "] AggregatorApp: StopApplication called." << std::endl;
  // Optional: Add cleanup here if needed, e.g., clearing remaining buffers
  // for (auto const& [key, val] : m_bufferMgr.m_map) { // C++17 structured binding
  //    val->CancelTimeoutEvent();
  // }
  // m_bufferMgr.m_map.clear(); // Or implement a Clear() method in AggBufferManager
}

} // namespace ndn
} // namespace ns3