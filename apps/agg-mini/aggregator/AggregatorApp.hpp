#ifndef NDN_AGG_APP_H
#define NDN_AGG_APP_H

#include "../../ndn-app.hpp"
#include "../buffer/AggBufferManager.hpp"   // <<< add
#include "../common/CommonUtils.hpp"
#include "helper/ndn-fib-helper.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "ns3/uinteger.h"                   // <<< add for UintegerValue
#include "ns3/nstime.h"
#include "ns3/string.h"
#include <vector>
#include <string>

namespace ns3 {
namespace ndn {

class AggregatorApp : public App {
public:
  static TypeId GetTypeId();
  AggregatorApp();

protected:
  virtual void StartApplication() override;
  virtual void OnInterest(std::shared_ptr<const Interest> interest) override;
  virtual void OnData(std::shared_ptr<const Data> data) override;
  virtual void StopApplication() override;

private:
  void OnStragglerTimeout(uint32_t seq);

  Name                       m_downPrefix;
  std::string                m_childPrefixesRaw;
  std::vector<Name>          m_childPrefixes;

  uint32_t                   m_bufferCapacity;      // max concurrent seqs
  Time                       m_stragglerTimeout;    // timeout for stragglers
  AggBufferManager           m_bufferMgr;           // <<< manager instance
};

} // namespace ndn
} // namespace ns3

#endif // NDN_AGG_APP_H