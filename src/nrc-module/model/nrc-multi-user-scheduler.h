#ifndef NRC_MULTI_USER_SCHEDULER_H
#define NRC_MULTI_USER_SCHEDULER_H

#include "ns3/rr-multi-user-scheduler.h"

// Add a doxygen group for this module.
// If you have more than one file, this should be in only one of them.
/**
 * \defgroup nrc-module Description of the nrc-module
 */

namespace ns3
{

class NrcMultiUserScheduler : public RrMultiUserScheduler
{
  public:
    static TypeId GetTypeId();
    NrcMultiUserScheduler();
    ~NrcMultiUserScheduler();
    Mac48Address& GetFirstAssocStaList();
    bool CheckLastTxIsDlMu();
    void NotifyAccessGranted(u_int8_t linkId);

  protected:
    void DoDispose() override;

  private:
    void AccessReqTimeout();

    // private:
    //   TxFormat SelectTxFormat() override;
    //   DlMuInfo ComputeDlMuInfo() override;
    //   UlMuInfo ComputeUlMuInfo() override;
};

// Each class should be documented using Doxygen,
// and have an \ingroup nrc-module directive

/* ... */

} // namespace ns3

#endif /* NRC_MULTI_USER_SCHEDULER_H */
