// LeafApp.hpp
#ifndef NDN_LEAF_APP_H
#define NDN_LEAF_APP_H

#include "../../ndn-producer.hpp"

namespace ns3 {
namespace ndn {

/**
 * \brief “Leaf” node app: simple Producer
 */
class LeafApp : public Producer {
  public:
    static TypeId GetTypeId();
    LeafApp();
    virtual ~LeafApp(); // <<< Add this declaration
  
  protected:
    virtual void StartApplication() override;
    virtual void StopApplication() override;
    virtual void OnInterest(shared_ptr<const Interest> interest) override;
  };

} // namespace ndn
} // namespace ns3

#endif // NDN_LEAF_APP_H
