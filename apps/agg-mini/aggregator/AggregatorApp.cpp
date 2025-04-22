#include "AggregatorApp.hpp"
#include "ns3/object.h"                // for NS_OBJECT_ENSURE_REGISTERED
#include "helper/ndn-fib-helper.hpp"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"              // <<< add
#include "ns3/nstime.h"
#include <sstream>

#include <ndn-cxx/util/time.hpp>        // for ndn::time::milliseconds
#include <ndn-cxx/encoding/buffer.hpp>    // for ndn::Buffer
#include <ndn-cxx/signature-info.hpp>     // for ndn::SignatureInfo
#include <ndn-cxx/encoding/estimator.hpp> // for ndn::EncodingEstimator
#include <ndn-cxx/encoding/buffer.hpp>    // for ndn::EncodingBuffer

NS_LOG_COMPONENT_DEFINE("ndn.AggregatorApp");

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
  , m_bufferMgr(0, Seconds(0))
{
}

void
AggregatorApp::StartApplication()
{
  App::StartApplication();
  FibHelper::AddRoute(GetNode(), m_downPrefix, m_face, 0);
  std::istringstream iss(m_childPrefixesRaw);
  std::string token;
  while (iss >> token) {
    m_childPrefixes.emplace_back(token);
  }
  // init manager with real attrs
  m_bufferMgr = AggBufferManager(m_bufferCapacity, m_stragglerTimeout);

  std::string nodeName = Names::FindName(GetNode());
  NS_LOG_INFO("[" << nodeName << "] Serving " << m_downPrefix
               << " → children={" << m_childPrefixesRaw << "}");
}

void
AggregatorApp::OnInterest(std::shared_ptr<const Interest> interest)
{
  App::OnInterest(interest);
  if (!m_active) return;

  uint32_t seq = interest->getName().get(-1).toSequenceNumber();
  std::string nodeName = Names::FindName(GetNode());
  NS_LOG_INFO("[" << nodeName << "] RX Interest " << interest->getName());

  if (!m_bufferMgr.Insert(seq,
                          interest->getName(),
                          m_childPrefixes.size(),
                          MakeCallback(&AggregatorApp::OnStragglerTimeout, this)))
  {
    NS_LOG_WARN("[" << nodeName << "] buffer full or dup seq=" << seq);
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
    childInterest->setCanBePrefix(false);
    childInterest->setNonce(interest->getNonce());
    childInterest->setInterestLifetime(interest->getInterestLifetime());

    NS_LOG_INFO("[" << nodeName << "] FWD to " << childName);
    m_transmittedInterests(childInterest, this, m_face);
    m_appLink->onReceiveInterest(*childInterest);
  }
}

void
AggregatorApp::OnData(std::shared_ptr<const Data> data)
{
  App::OnData(data);
  if (!m_active) return;

  uint32_t seq = data->getName().get(-1).toSequenceNumber();
  std::string nodeName = Names::FindName(GetNode());
  NS_LOG_INFO("[" << nodeName << "] RX Data " << data->getName());

  auto buf = m_bufferMgr.Get(seq);
  if (!buf) {
    NS_LOG_WARN("[" << nodeName << "] no buffer for seq=" << seq);
    return;
  }

  // extract integer payload
  int value = 0;
  {
    const ::ndn::Block& block = data->getContent();
    size_t copyLen = std::min(block.value_size(), sizeof(value));
    std::memcpy(&value, block.value(), copyLen);
  }

  buf->AddValue(value);
  buf->IncrementResponse();

  if (buf->IsComplete()) {
    Name upName = buf->GetParentInterestName();
    auto aggData = std::make_shared<Data>();
    aggData->setName(upName);
    aggData->setFreshnessPeriod(ndn::time::milliseconds(0));

    // marshal the sum into a local so & is valid
    int sum = buf->GetSum();
    auto bufPtr = std::make_shared<::ndn::Buffer>(
      reinterpret_cast<const uint8_t*>(&sum),
      sizeof(sum));
    ::ndn::Block content(::ndn::tlv::Content, bufPtr);
    aggData->setContent(content);
    ns3::ndn::common::SetDummySignature(aggData);
    aggData->wireEncode();

    NS_LOG_INFO("[" << nodeName << "] AGG send up " << upName);
    m_transmittedDatas(aggData, this, m_face);
    m_appLink->onReceiveData(*aggData);

    m_bufferMgr.Remove(seq);
  }
}

void
AggregatorApp::OnStragglerTimeout(uint32_t seq)
{
  std::string nodeName = Names::FindName(GetNode());
  NS_LOG_WARN("[" << nodeName << "] straggler timeout seq=" << seq);

  auto buf = m_bufferMgr.Get(seq);
  if (!buf || buf->HasReplied()) return;

  Name upName = buf->GetParentInterestName();
  auto aggData = std::make_shared<Data>();
  aggData->setName(upName);
  aggData->setFreshnessPeriod(ndn::time::milliseconds(0));

  // local sum for addressability
  int sum = buf->GetSum();
  auto bufPtr2 = std::make_shared<::ndn::Buffer>(
                    reinterpret_cast<const uint8_t*>(&sum),
                    sizeof(sum));
  ::ndn::Block content2(::ndn::tlv::Content, bufPtr2);

  aggData->setContent(content2);
  ns3::ndn::common::SetDummySignature(aggData);
  aggData->wireEncode();

  NS_LOG_INFO("[" << nodeName << "] Timeout-AGG send up " << upName);
  m_transmittedDatas(aggData, this, m_face);
  m_appLink->onReceiveData(*aggData);

  buf->MarkReplied();
  m_bufferMgr.Remove(seq);
}

void
AggregatorApp::StopApplication()
{
  App::StopApplication();
  std::string nodeName = Names::FindName(GetNode());
  NS_LOG_INFO("[" << nodeName << "] Stopped");
}

} // namespace ndn
} // namespace ns3