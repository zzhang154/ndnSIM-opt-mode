#ifndef AGGREGATION_BUFFER_HPP
#define AGGREGATION_BUFFER_HPP

#include <vector>
#include <cstdint>
#include "ns3/event-id.h"
#include "ndn-cxx/name.hpp"

/** AggregationBuffer: holds partial aggregate data for one sequence round. */
struct AggregationBuffer {
  uint32_t expectedCount;               // number of children expected
  uint32_t receivedCount;               // number of children responses received so far
  uint64_t partialSum;                  // aggregated sum of child values received
  ns3::EventId timeoutEvent;           // scheduled timeout event for this round
  ndn::Name parentInterestName;        // the Interest name from the parent (for aggregator nodes)
  std::vector<bool> childrenReceived;  // flags to mark which children have responded

  AggregationBuffer(uint32_t expCount = 0)
    : expectedCount(expCount)
    , receivedCount(0)
    , partialSum(0) {
    if (expCount > 0) {
      childrenReceived.resize(expCount, false);
    }
  }
};

#endif // AGGREGATION_BUFFER_HPP
