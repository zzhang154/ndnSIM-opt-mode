// RootApp.hpp
#ifndef NDN_ROOT_APP_H
#define NDN_ROOT_APP_H

#include "../../ndn-consumer.hpp"

namespace ns3 {
namespace ndn {

/**
 * \brief "Root" node app: inherits from Consumer, sends a fixed number of Interests.
 */
class RootApp : public Consumer {
public:
  static TypeId GetTypeId();
  RootApp();
  virtual ~RootApp(); // <<< Add this declaration

  // Add MaxSeq accessors
  uint32_t GetMaxSeq() const;
  void SetMaxSeq(uint32_t maxSeq);

  // <<< Add these missing declarations >>>
  virtual void StartApplication() override;
  virtual void StopApplication() override;
  virtual void OnData(std::shared_ptr<const Data> data) override;
  // <<< End of added declarations >>>

  // Override in the public section to match Consumer class
  virtual void SendPacket();  // Removed 'override' keyword since base method isn't virtual

protected:
  /// Called by Consumer to schedule the next Interest
  virtual void ScheduleNextPacket() override;

private:
  double m_interval; ///< seconds between Interests
};

} // namespace ndn
} // namespace ns3

#endif // NDN_ROOT_APP_H