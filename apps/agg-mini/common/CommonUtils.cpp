#include "CommonUtils.hpp"
#include <ndn-cxx/signature-info.hpp>
#include <ndn-cxx/encoding/estimator.hpp>
#include <ndn-cxx/encoding/buffer.hpp>

namespace ns3 {
namespace ndn {
namespace common {

void
SetDummySignature(const std::shared_ptr<Data>& data)
{
  // 1) fake SignatureInfo
  ::ndn::SignatureInfo sigInfo(
    static_cast<::ndn::tlv::SignatureTypeValue>(255)
  );
  data->setSignatureInfo(sigInfo);

  // 2) zero-length SignatureValue
  ::ndn::EncodingEstimator estimator;
  ::ndn::EncodingBuffer encoder(
    estimator.appendVarNumber(0), 0);
  encoder.appendVarNumber(0);
  data->setSignatureValue(encoder.getBuffer());
}

} // namespace common
} // namespace ndn
} // namespace ns3