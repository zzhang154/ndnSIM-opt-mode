#ifndef AGG_MINI_COMMON_UTILS_HPP
#define AGG_MINI_COMMON_UTILS_HPP

#include "model/ndn-common.hpp"  // brings ::ndn::Data into ns3::ndn
#include <memory>

namespace ns3 {
namespace ndn {
namespace common {

/** Attach an empty SignatureInfo+SignatureValue so wireEncode() won’t throw */
void
SetDummySignature(const std::shared_ptr<Data>& data);

} // namespace common
} // namespace ndn
} // namespace ns3

#endif // AGG_MINI_COMMON_UTILS_HPP