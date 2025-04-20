#ifndef CFN_ROOT_APP_HPP
#define CFN_ROOT_APP_HPP

#include "../ndn-consumer.hpp" // We still inherit from Consumer for other functionality

namespace ns3 {
namespace ndn {

class CFNRootApp : public Consumer
{
public:
  static TypeId GetTypeId(void);
  CFNRootApp();
  virtual ~CFNRootApp();

protected:
  virtual void StartApplication() override;
  virtual void OnData(shared_ptr<const Data> data) override;
  virtual void ScheduleNextPacket() override;
  
  // Add our custom SendPacket method
  virtual void SendPacket();

private:
  uint32_t m_rounds;
  uint32_t m_currentRound;
};

} // namespace ndn
} // namespace ns3

#endif // CFN_ROOT_APP_HPP