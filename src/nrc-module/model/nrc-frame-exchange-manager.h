#ifndef NRC_FRAME_EXCHANGE_MANAGER_H
#define NRC_FRAME_EXCHANGE_MANAGER_H

#include "nrc-ctrl-headers.h"

#include "ns3/eht-frame-exchange-manager.h"

// Add a doxygen group for this module.
// If you have more than one file, this should be in only one of them.
/**
 * \defgroup nrc-module Description of the nrc-module
 */

namespace ns3
{

enum TxsTime : bool
{
    NOT_ENOUGH = false,
    ENOUGH = true
};

struct TxsParams
{
    Time txsDuration{0};
    Time txsStart{0};
};

class NrcFrameExchangeManager : public EhtFrameExchangeManager
{
  public:
    static TypeId GetTypeId();
    NrcFrameExchangeManager();
    ~NrcFrameExchangeManager() override;

  protected:
    void DoDispose() override;

    bool TrySendMuRtsTxs(const Time availableTime,
                         const Time txDuration,
                         const WifiTxVector& ctsTxVector) const;

    bool SendMuRtsTxs(const WifiTxParameters& txParams);

    bool StartFrameExchange(Ptr<QosTxop> edca, Time availableTime, bool initialFrame) override;
    bool SendMuRtsTxs(const Mac48Address& receiver, const Time availableTime);
    WifiTxVector GetCtsTxVectorAfterMuRts(const NrcCtrlTriggerHeader& trigger,
                                          uint16_t staId) const;
    void CtsAfterMuRtsTxsTimeout(Ptr<WifiMpdu> muRts, const WifiTxVector& txVector);
    void CheckReGrantConditions(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector);

    Time CalculateMuRtsTxDuration(const NrcCtrlTriggerHeader& muRtsTxs,
                                  const WifiTxVector& muRtsTxsTxVector,
                                  const WifiMacHeader& hdr) const;
    void ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                     RxSignalInfo rxSignalInfo,
                     const WifiTxVector& txVector,
                     bool inAmpdu) override;
    void EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                         const RxSignalInfo& rxSignalInfo,
                         const WifiTxVector& txVector,
                         const std::vector<bool>& perMpduStatus) override;
    void SendCtsAfterMuRtsTxs(const WifiMacHeader& muRtsHdr,
                              const NrcCtrlTriggerHeader& trigger,
                              double muRtsSnr);
    bool IsInValid(Ptr<const WifiMpdu> mpdu) const;
    void SetImaginaryPsdu();
    bool StartTransmission(Ptr<QosTxop> edca, uint16_t allowedWidth);
    void SetSharedStaAddress(Mac48Address macAddress);
    Mac48Address GetSharedStaAddress() const;
    Time GetBlockAckDuration(const RecipientBlockAckAgreement& agreement,
                             Time durationId,
                             WifiTxVector& blockAckTxVector,
                             double rxSnr);
    void SendBlockAck(const RecipientBlockAckAgreement& agreement,
                      Time durationId,
                      WifiTxVector& blockAckTxVector,
                      double rxSnr);
    void SendNormalAck(const WifiMacHeader& hdr, const WifiTxVector& dataTxVector, double dataSnr);
    void SetTxsParams(Time txsDuration);
    TxsParams GetTxsParams();
    Time GetRemainingTxsDuration() const;
    void ResetTxTimer(Ptr<const WifiMpdu> mpdu,
                      const WifiTxVector& txVector,
                      const Mac48Address& macAddress,
                      Time duration);
    void PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector);

  private:
    WifiTxParameters m_txParams; //!< the TX parameters for the MU-RTX TXS frame;
    Mac48Address m_sharedStaAddress;
    TxsParams m_txsParams;
    uint8_t GetTidFromAc(AcIndex ac);
};

// Each class should be documented using Doxygen,
// and have an \ingroup nrc-module directive

/* ... */

} // namespace ns3

#endif /* NRC_FRAME_EXCHANGE_MANAGER_H */
