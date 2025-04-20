#ifndef CFN_AGG_CONFIG_HPP
#define CFN_AGG_CONFIG_HPP

#include <string>
#include <map>

namespace ns3 {
namespace ndn {

class CFNAggConfig {
public:
  CFNAggConfig();

  // Load configuration from a file. Returns true on success.
  bool LoadFromFile(const std::string &filename);

  // Get the number of aggregation rounds.
  uint32_t GetRounds() const;

  // Get the partial aggregation timeout (in milliseconds).
  uint32_t GetPartialTimeout() const;

  // Get the producer payload (numeric value) for a given producer node.
  uint32_t GetProducerPayload(const std::string &producerName) const;

  // Get the simulation stop time (in seconds).
  uint32_t GetStopTime() const;

private:
  uint32_t m_rounds;
  uint32_t m_partialTimeout; // in milliseconds
  uint32_t m_stopTime;       // in seconds
  std::map<std::string, uint32_t> m_producerPayloads;

  // Helper function to trim whitespace from a string.
  std::string Trim(const std::string &s);
};

} // namespace ndn
} // namespace ns3

#endif // CFN_AGG_CONFIG_HPP
