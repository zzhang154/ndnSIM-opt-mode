#include "CFNAggConfig.hpp"
#include "ns3/log.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

NS_LOG_COMPONENT_DEFINE("CFNAggConfig");

namespace ns3 {
namespace ndn {

CFNAggConfig::CFNAggConfig()
  : m_rounds(5)
  , m_partialTimeout(20)
  , m_stopTime(10) // Default: simulation stops at 10s if not overridden.
{
}

bool
CFNAggConfig::LoadFromFile(const std::string &filename)
{
  std::ifstream infile(filename.c_str());
  if (!infile.is_open()) {
    NS_LOG_ERROR("CFNAggConfig: Cannot open config file " << filename);
    return false;
  }

  std::string line;
  while (std::getline(infile, line)) {
    // Trim and skip comments or empty lines.
    line = Trim(line);
    if (line.empty() || line[0] == '#')
      continue;
    std::istringstream iss(line);
    std::string key, value;
    if (std::getline(iss, key, '=') && std::getline(iss, value)) {
      key = Trim(key);
      value = Trim(value);
      if (key == "rounds") {
        m_rounds = static_cast<uint32_t>(std::stoi(value));
      } else if (key == "partialTimeout") {
        // Remove a trailing "ms" if present.
        size_t pos = value.find("ms");
        if (pos != std::string::npos)
          value = value.substr(0, pos);
        m_partialTimeout = static_cast<uint32_t>(std::stoi(value));
      } else if (key == "stopTime") {
        // Remove a trailing "s" if present.
        size_t pos = value.find("s");
        if (pos != std::string::npos)
          value = value.substr(0, pos);
        m_stopTime = static_cast<uint32_t>(std::stoi(value));
      } else if (key.find(".value") != std::string::npos) {
        // Expect keys like "Leaf1.value" or "Leaf2.value".
        std::string producerName = key.substr(0, key.find(".value"));
        uint32_t payload = static_cast<uint32_t>(std::stoi(value));
        m_producerPayloads[producerName] = payload;
      } else {
        NS_LOG_WARN("CFNAggConfig: Unknown key \"" << key << "\" in configuration file.");
      }
    }
  }
  infile.close();
  NS_LOG_INFO("CFNAggConfig: Loaded configuration: rounds=" << m_rounds
              << ", partialTimeout=" << m_partialTimeout << "ms, stopTime=" << m_stopTime << "s.");
  return true;
}

uint32_t
CFNAggConfig::GetRounds() const
{
  return m_rounds;
}

uint32_t
CFNAggConfig::GetPartialTimeout() const
{
  return m_partialTimeout;
}

uint32_t
CFNAggConfig::GetStopTime() const
{
  return m_stopTime;
}

uint32_t
CFNAggConfig::GetProducerPayload(const std::string &producerName) const
{
  auto it = m_producerPayloads.find(producerName);
  if (it != m_producerPayloads.end())
    return it->second;
  return 1; // Default payload value if not specified.
}

std::string
CFNAggConfig::Trim(const std::string &s)
{
  std::string result = s;
  // Left trim.
  result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
  // Right trim.
  result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), result.end());
  return result;
}

} // namespace ndn
} // namespace ns3
