#ifndef AGG_BUFFER_MANAGER_HPP
#define AGG_BUFFER_MANAGER_HPP

#include "AggBuffer.hpp"
#include "ns3/callback.h"
#include "ns3/simulator.h"
#include <map>
#include <memory>

namespace ns3 {
namespace ndn {

/**
 * \brief Manages multiple AggBuffer entries (one per sequence).
 * Enforces a maximum capacity (number of parallel sequences),
 * and schedules a timeout event per entry.
 */
class AggBufferManager {
public:
  /// Callback invoked when a sequence times out (straggler detection)
  using TimeoutCallback = Callback<void, uint32_t>;

  /**
   * \param capacity    Max concurrent sequence buffers
   * \param timeout     Straggler timeout duration
   */
  AggBufferManager(uint32_t capacity, Time timeout);

  /// Can we insert a new sequence?
  bool CanInsert() const;

  /**
   * Insert a new buffer entry for seq.
   * \param seq           Sequence ID
   * \param parentName    Name of the incoming Interest
   * \param expectedCount Number of child contributions to wait for
   * \param onTimeout     Callback(seq) when straggler‐timeout fires
   * \returns true if inserted; false if capacity exceeded
   */
  bool Insert(uint32_t seq,
              const ::ndn::Name& parentName,
              uint32_t expectedCount,
              TimeoutCallback onTimeout);

  /// Retrieve the buffer entry for seq (or nullptr if missing)
  AggBuffer* Get(uint32_t seq) const;

  /// Erase and cancel timeout for seq
  void Remove(uint32_t seq);

private:
  uint32_t                                            m_capacity;
  Time                                                m_timeout;
  std::map<uint32_t, std::unique_ptr<AggBuffer>>      m_map;
};

} // namespace ndn
} // namespace ns3

#endif // AGG_BUFFER_MANAGER_HPP