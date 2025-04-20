// AggregatorApp.hpp
#ifndef NDN_AGG_APP_H
#define NDN_AGG_APP_H

#include "../ndn-app.hpp"
#include "helper/ndn-fib-helper.hpp"
#include "model/ndn-l3-protocol.hpp"

namespace ns3 {
namespace ndn {

/**
 * \brief Aggregator: on OnInterest from “Root” issues an Interest to Leaf,
 *        on OnData from Leaf repackages and returns Data upstream.
 */
class AggregatorApp : public App {
public:
  static TypeId GetTypeId();
  AggregatorApp();
  virtual ~AggregatorApp() {}

protected:
  virtual void StartApplication() override;
  virtual void StopApplication() override;

  virtual void OnInterest(shared_ptr<const Interest> interest) override;
  virtual void OnData(shared_ptr<const Data> data) override;

private:
  Name m_downPrefix;  ///< prefix we serve to Root
  Name m_childPrefix; ///< prefix we query from Leaf
};

} // namespace ndn
} // namespace ns3

#endif // NDN_AGG_APP_H
