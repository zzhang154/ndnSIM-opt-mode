#ifndef CFN_AGGREGATOR_APP_HPP
#define CFN_AGGREGATOR_APP_HPP

#include "../apps/ndn-app.hpp"
#include "AggregationBuffer.hpp" // Include the separate class
#include "ns3/simulator.h"
#include <vector>
#include <map>

namespace ns3 {
namespace ndn {

class CFNAggregatorApp : public App
{
public:
  static TypeId GetTypeId(void);
  CFNAggregatorApp();
  virtual ~CFNAggregatorApp();

  // Add a child prefix for which this aggregator will forward Interests.
  void AddChildPrefix(const Name& prefix);

  // Set the partial aggregation timeout.
  void SetPartialTimeout(Time timeout) { m_partialTimeout = timeout; }

protected:
  virtual void StartApplication() override;
  virtual void StopApplication() override;

  // Handle incoming Interest from parent.
  virtual void OnInterest(shared_ptr<const Interest> interest) override;

  // Handle Data packets coming from child nodes.
  virtual void OnData(shared_ptr<const Data> data) override;

private:
  // application's prefix
  Name m_prefix;

  // Map sequence numbers to AggregationBuffer objects
  std::map<uint32_t, AggregationBuffer> m_aggBufferMap;

  // List of child prefixes.
  std::vector<Name> m_childrenPrefixes;

  // Partial aggregation timeout.
  Time m_partialTimeout;

  // Helper to forward an Interest to all child nodes.
  void ForwardInterestToChildren(uint32_t seq);

  // Timeout handler for aggregation rounds.
  void AggregationTimeout(uint32_t seq);

  // Extract the sequence number (assumed to be the last name component).
  uint32_t ExtractSequenceNumber(const Name& name);
};

} // namespace ndn
} // namespace ns3

#endif // CFN_AGGREGATOR_APP_HPP