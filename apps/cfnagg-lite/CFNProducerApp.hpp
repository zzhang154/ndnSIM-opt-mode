#ifndef CFN_PRODUCER_APP_HPP
#define CFN_PRODUCER_APP_HPP

#include "../ndn-producer.hpp"

namespace ns3 {
namespace ndn {

class CFNProducerApp : public Producer
{
public:
  static TypeId GetTypeId(void);
  CFNProducerApp();
  virtual ~CFNProducerApp();

  // Set the numeric payload value
  void SetPayloadValue(uint32_t value) { m_payloadValue = value; }

protected:
  virtual void OnInterest(shared_ptr<const Interest> interest) override;

private:
  uint32_t m_payloadValue;
};

} // namespace ndn
} // namespace ns3

#endif // CFN_PRODUCER_APP_HPP
