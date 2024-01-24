/*
 * Copyright (c) 2024 Newracom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Seungmin Lee <sm.lee@newratek.com>
 *         Changmin Lee <cm.lee@newratek.com>
 */

#include "txs-frame-exchange-manager.h"

#include "txs-multi-user-scheduler.h"

#include "ns3/abort.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/erp-ofdm-phy.h"
#include "ns3/log.h"
#include "ns3/qos-txop.h"
#include "ns3/snr-tag.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-tx-vector.h"

// #include "ns3/multi-user-scheduler.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                                                      \
    std::clog << "[Time=" << Simulator::Now().GetMicroSeconds() << "]"                             \
              << "[link=" << +m_linkId << "][mac=" << m_self << "] "

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TxsFrameExchangeManager");
NS_OBJECT_ENSURE_REGISTERED(TxsFrameExchangeManager);

TypeId
TxsFrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TxsFrameExchangeManager")
                            .SetParent<EhtFrameExchangeManager>()
                            .AddConstructor<TxsFrameExchangeManager>()
                            .SetGroupName("Wifi");
    return tid;
}

TxsFrameExchangeManager::TxsFrameExchangeManager()
{
    NS_LOG_FUNCTION(this);
}

TxsFrameExchangeManager::~TxsFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
TxsFrameExchangeManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_txParams.Clear();
    EhtFrameExchangeManager::DoDispose();
}

bool
TxsFrameExchangeManager::StartFrameExchange(Ptr<QosTxop> edca,
                                            Time availableTime,
                                            bool initialFrame)
{
    // NS_LOG_INFO(this << *mpdu << rxSignalInfo << txVector << inAmpdu);
    NS_LOG_INFO("AvailableTime: " << availableTime.GetMicroSeconds());
    if (this->m_apMac)
    {
        NS_LOG_INFO("Is AP: " << this->m_apMac->GetAddress());
    }
    if (this->m_staMac)
    {
        NS_LOG_INFO("Is STA: " << this->m_staMac->GetAddress());
        return HeFrameExchangeManager::StartFrameExchange(edca, availableTime, initialFrame);
    }

    Ptr<const WifiMpdu> mpdu;
    mpdu = edca->PeekNextMpdu(m_linkId);

    if (this->GetMuScheduler())
    {
        Ptr<TxsMultiUserScheduler> txsMuScheduler =
            StaticCast<TxsMultiUserScheduler>(this->GetMuScheduler());
        bool lastTxIsDl = txsMuScheduler->CheckLastTxIsDlMu();
        NS_LOG_INFO("!GetBar(edca->GetAccessCategory()) :" << !GetBar(edca->GetAccessCategory()));
        NS_LOG_INFO("!(mpdu = edca->PeekNextMpdu(m_linkId)) :" << !(mpdu));
        NS_LOG_INFO("lastTxIsDl: " << lastTxIsDl);

        if (!GetBar(edca->GetAccessCategory()) &&
            (!(mpdu = edca->PeekNextMpdu(m_linkId)) || (lastTxIsDl)) &&
            availableTime.IsStrictlyPositive() && !initialFrame)
        {
            if (initialFrame)
            {
                txsMuScheduler->NotifyAccessGranted(m_linkId);
            }

            if (Mac48Address ulSta("00:00:00:00:00:02");
                txsMuScheduler->GetFirstAssocStaList() == ulSta)
            {
                NS_LOG_INFO("UL STA Address: " << txsMuScheduler->GetFirstAssocStaList());
                if (SendMuRtsTxs(txsMuScheduler->GetFirstAssocStaList(), availableTime) ==
                    TxsTime::ENOUGH)
                {
                    return true;
                }
                NS_LOG_INFO("TXS operating time is not enough");
            }
        }
    }
    return HeFrameExchangeManager::StartFrameExchange(edca, availableTime, initialFrame);
}

Time
TxsFrameExchangeManager::CalculateMuRtsTxDuration(const TxsCtrlTriggerHeader& muRtsTxs,
                                                  const WifiTxVector& muRtsTxsTxVector,
                                                  const WifiMacHeader& hdr) const
{
    Ptr<Packet> payload = Create<Packet>();
    payload->AddHeader(muRtsTxs);

    auto mpdu = Create<WifiMpdu>(payload, hdr);
    return m_phy->CalculateTxDuration(mpdu->GetSize(), muRtsTxsTxVector, m_phy->GetPhyBand());
}

void
TxsFrameExchangeManager::SetImaginaryPsdu()
{
    Ptr<Packet> imaginaryPacket = Create<Packet>();

    WifiMacHeader imaginaryHdr(WIFI_MAC_CTL_TRIGGER);

    auto imaginaryPsdu = Create<WifiPsdu>(imaginaryPacket, imaginaryHdr);
    HeFrameExchangeManager::m_psduMap.insert({0, imaginaryPsdu});
}

bool
TxsFrameExchangeManager::SendMuRtsTxs(const Mac48Address& receiver, const Time availableTime)
{
    NS_LOG_FUNCTION(this);
    Ptr<WifiRemoteStationManager> wifiRemoteStationManager = this->GetWifiRemoteStationManager();

    TxsCtrlTriggerHeader muRtsTxs;
    muRtsTxs.SetType(TriggerFrameType::MU_RTS_TRIGGER);
    muRtsTxs.SetCsRequired(true);
    muRtsTxs.SetTxsMode(TxsModes::MU_RTS_TXS_MODE_1);
    muRtsTxs.SetUlBandwidth(m_allowedWidth);
    muRtsTxs.AddUserInfoToMuRts(receiver, m_apMac, m_linkId, wifiRemoteStationManager);

    WifiTxVector muRtsTxsTxVector;
    muRtsTxsTxVector = wifiRemoteStationManager->GetRtsTxVector(receiver, m_allowedWidth);
    muRtsTxsTxVector.SetChannelWidth(m_allowedWidth);
    const auto modulation = muRtsTxsTxVector.GetModulationClass();
    if (modulation == WIFI_MOD_CLASS_DSSS || modulation == WIFI_MOD_CLASS_HR_DSSS)
    {
        muRtsTxsTxVector.SetMode(ErpOfdmPhy::GetErpOfdmRate6Mbps());
    }

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_TRIGGER);
    hdr.SetAddr1(Mac48Address::GetBroadcast());
    hdr.SetAddr2(m_self);
    hdr.SetDsNotTo();
    hdr.SetDsNotFrom();
    hdr.SetNoRetry();
    hdr.SetNoMoreFragments();
    hdr.SetQosTid(GetTidFromAc(m_edca->GetAccessCategory()));

    Time txDuration = CalculateMuRtsTxDuration(muRtsTxs, muRtsTxsTxVector, hdr);
    Time sharedTxopDuration = availableTime - txDuration;
    if (sharedTxopDuration.IsStrictlyNegative())
    {
        return TxsTime::NOT_ENOUGH;
    }
    NS_LOG_INFO("MU-RTS TXS frame duration = " << txDuration.GetMicroSeconds());

    uint8_t aid12 = m_apMac->GetAssociationId(receiver, m_linkId);
    auto userInfoIt = std::find_if(muRtsTxs.begin(),
                                   muRtsTxs.end(),
                                   [aid12](const TxsCtrlTriggerUserInfoField& ui) -> bool {
                                       return (ui.GetAid12() == aid12);
                                   });
    NS_ASSERT_MSG(userInfoIt != muRtsTxs.end(),
                  "User Info field for AID=" << aid12 << " not found");

    userInfoIt->SetAllocationDuration(sharedTxopDuration);
    SetTxsParams(sharedTxopDuration + txDuration);
    NS_LOG_INFO("This User Info field for AID = " << userInfoIt->GetAid12());
    NS_LOG_INFO("Sharing STA (" << m_apMac->GetAddress() << ") sends to shared STA (" << receiver
                                << ")");

    Ptr<Packet> payload = Create<Packet>();
    payload->AddHeader(muRtsTxs);

    auto mpdu = Create<WifiMpdu>(payload, hdr);
    mpdu->GetHeader().SetDuration(sharedTxopDuration); // Duration/ID field fill to sharing txop
                                                       // duration (nanosecond) Full protection

    WifiTxVector ctsTxVector = GetCtsTxVectorAfterMuRts(muRtsTxs, aid12);
    if (TrySendMuRtsTxs(availableTime, txDuration, ctsTxVector) == TxsTime::NOT_ENOUGH)
    {
        return TxsTime::NOT_ENOUGH;
    }

    // After transmitting an MU-RTS frame, the STA shall wait for a CTSTimeout interval of
    // aSIFSTime + aSlotTime + aRxPHYStartDelay (Sec. 27.2.5.2 of 802.11ax D3.0).
    // aRxPHYStartDelay equals the time to transmit the PHY header.
    Time timeout = txDuration + m_phy->GetSifs() + m_phy->GetSlot() +
                   m_phy->CalculatePhyPreambleAndHeaderDuration(ctsTxVector);

    NS_ASSERT(!m_txTimer.IsRunning());
    NS_LOG_INFO("Current Time: " << Simulator::Now().GetMicroSeconds());
    NS_LOG_INFO("CTS Timeout when transmitting MU-RTS TXS: " << timeout.GetMicroSeconds());

    m_txTimer.Set(WifiTxTimer::WAIT_CTS_AFTER_MU_RTS_TXS_MODE_1,
                  timeout,
                  {receiver},
                  &TxsFrameExchangeManager::CtsAfterMuRtsTxsTimeout,
                  this,
                  mpdu,
                  muRtsTxsTxVector);

    m_channelAccessManager->NotifyCtsTimeoutStartNow(timeout);

    SetImaginaryPsdu();
    SetSharedStaAddress(receiver);
    m_sentRtsTo = {receiver};
    ForwardMpduDown(mpdu, muRtsTxsTxVector);
    return TxsTime::ENOUGH;
}

bool
TxsFrameExchangeManager::TrySendMuRtsTxs(const Time availableTime,
                                         const Time txDuration,
                                         const WifiTxVector& ctsTxVector) const
{
    NS_LOG_FUNCTION(this);

    Time ctsTxDuration = m_phy->CalculateTxDuration(GetCtsSize(), ctsTxVector, m_phy->GetPhyBand());
    Time frameExchangeDuration = txDuration + m_phy->GetSifs() * 2 + ctsTxDuration;

    return (availableTime - frameExchangeDuration).IsStrictlyPositive() ? TxsTime::ENOUGH
                                                                        : TxsTime::NOT_ENOUGH;
}

bool
TxsFrameExchangeManager::IsInValid(Ptr<const WifiMpdu> mpdu) const
{
    TxsCtrlTriggerHeader trigger(false);
    mpdu->GetPacket()->PeekHeader(trigger);
    return !(trigger.IsMuRts()) || (trigger.GetTxsMode() != TxsModes::MU_RTS_TXS_MODE_1 &&
                                    trigger.GetTxsMode() != TxsModes::MU_RTS_TXS_MODE_2);
}

void
TxsFrameExchangeManager::ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                                     RxSignalInfo rxSignalInfo,
                                     const WifiTxVector& txVector,
                                     bool inAmpdu)
{
    NS_LOG_INFO(this << *mpdu << rxSignalInfo << txVector << inAmpdu);
    // The received MPDU is either broadcast or addressed to this station
    NS_ASSERT(mpdu->GetHeader().GetAddr1().IsGroup() || mpdu->GetHeader().GetAddr1() == m_self);

    const WifiMacHeader& hdr = mpdu->GetHeader();

    if (hdr.IsCtl())
    {
        if (hdr.IsCts())
        {
            if (m_txTimer.IsRunning() &&
                (m_txTimer.GetReason() == WifiTxTimer::WAIT_CTS_AFTER_MU_RTS_TXS_MODE_1) &&
                (hdr.GetAddr1() == m_self))
            {
                Mac48Address sender = *(m_txTimer.GetStasExpectedToRespond().begin());
                NS_LOG_INFO(
                    "AP (" << m_self
                           << ") that is in WAIT_CTS_AFTER_MU_RTS_TXS_MODE_1 receives the CTS from "
                              "shared STA ("
                           << sender << ")");
                NS_LOG_INFO(
                    "Current Time when receiving CTS: " << Simulator::Now().GetMicroSeconds());

                SnrTag tag;
                mpdu->GetPacket()->PeekPacketTag(tag);
                GetWifiRemoteStationManager()->ReportRxOk(sender, rxSignalInfo, txVector);
                GetWifiRemoteStationManager()->ReportRtsOk(mpdu->GetHeader(),
                                                           rxSignalInfo.snr,
                                                           txVector.GetMode(),
                                                           tag.Get());

                m_txTimer.Cancel();
                m_channelAccessManager->NotifyCtsTimeoutResetNow();
                m_psduMap.clear();

                ResetTxTimer(mpdu, txVector, sender, m_phy->GetPifs());

                m_protectedStas.merge(m_sentRtsTo);
                m_sentRtsTo.clear();
                return;
            }
        }
        else if (hdr.IsTrigger())
        {
            // Trigger Frames are only processed by STAs
            if (!m_staMac)
            {
                return;
            }

            TxsCtrlTriggerHeader commonInfoField(false);
            mpdu->GetPacket()->PeekHeader(commonInfoField);
            if ((commonInfoField.GetType() == TriggerFrameType::MU_RTS_TRIGGER) &&
                (commonInfoField.GetTxsMode() == TxsModes::MU_RTS_TXS_MODE_1))
            {
                TxsCtrlTriggerHeader trigger;
                mpdu->GetPacket()->PeekHeader(trigger);

                if (hdr.GetAddr2() != m_bssid // not sent by the AP this STA is associated with
                    || trigger.FindUserInfoWithAid(m_staMac->GetAssociationId()) == trigger.end())
                {
                    // not addressed to us
                    return;
                }

                NS_LOG_INFO("AID = " << m_staMac->GetAssociationId()
                                     << " is in the received User Info field.");
                NS_LOG_INFO("Address of shared STA that find own User info field from the received "
                            "MU-RTS TXS is "
                            << m_staMac->GetAddress());

                Mac48Address sender = hdr.GetAddr2();
                GetWifiRemoteStationManager()->ReportRxOk(sender, rxSignalInfo, txVector);

                NS_LOG_DEBUG("Schedule CTS After sending MU-RTS TXS frame");
                Simulator::Schedule(m_phy->GetSifs(),
                                    &TxsFrameExchangeManager::SendCtsAfterMuRtsTxs,
                                    this,
                                    hdr,
                                    trigger,
                                    rxSignalInfo.snr);
                return;
            }
        }
        else if (hdr.IsBlockAck() && m_txTimer.IsRunning() && protectedFromMuRtsTxs &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_BLOCK_ACK)
        {
            NS_ASSERT(m_edca); // Recipient of BA must set m_edca
            m_edca->StartMuEdcaTimerNow(m_linkId);

            HeFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
            return;
        }
        else if (hdr.IsAck() && m_txTimer.IsRunning() && protectedFromMuRtsTxs &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_NORMAL_ACK)
        {
            NS_ASSERT(m_edca); // Recipient of BA must set m_edca

            m_edca->StartMuEdcaTimerNow(m_linkId);

            HeFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
            return;
        }
    }

    if (hdr.IsQosData() && hdr.HasData() && hdr.GetAddr1() == m_self)
    {
        Mac48Address sender = *(m_txTimer.GetStasExpectedToRespond().begin());
        if (m_txTimer.IsRunning() && (hdr.GetAddr2() == sender) &&
            (m_txTimer.GetReason() == WifiTxTimer::WAIT_REGRANT_AFTER_TXS))
        {
            uint8_t tid = hdr.GetQosTid();
            if (m_mac->GetBaAgreementEstablishedAsRecipient(hdr.GetAddr2(), tid))
            {
                if (!inAmpdu && hdr.GetQosAckPolicy() == WifiMacHeader::NORMAL_ACK)
                {
                    WifiTxVector ackTxVector =
                        GetWifiRemoteStationManager()->GetAckTxVector(hdr.GetAddr2(), txVector);

                    Time ackTxDuration =
                        m_phy->GetSifs() +
                        m_phy->CalculateTxDuration(GetAckSize(), ackTxVector, m_phy->GetPhyBand());

                    m_txTimer.Cancel();
                    if (GetRemainingTxsDuration() > ackTxDuration + m_phy->GetPifs())
                    {
                        ResetTxTimer(mpdu, txVector, sender, ackTxDuration + m_phy->GetPifs());
                    }
                    else
                    {
                        ResetTxTimer(mpdu, txVector, sender, ackTxDuration + m_phy->GetSifs());
                    }
                }
                else
                {
                    m_txTimer.Cancel();
                    Time lastBusy =
                        m_channelAccessManager->GetAccessGrantStart(false) - m_phy->GetSifs();
                    Time lastBusyEnd = std::max(lastBusy, m_navEnd);

                    ResetTxTimer(mpdu, txVector, sender, lastBusyEnd + m_phy->GetPifs());
                }
            }
        }
        HeFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
        return;
    }
    else if (hdr.IsData() && !hdr.IsQosData() && hdr.GetAddr1() == m_self)
    {
        Mac48Address sender = *(m_txTimer.GetStasExpectedToRespond().begin());
        if (m_txTimer.IsRunning() && (hdr.GetAddr2() == sender) &&
            (m_txTimer.GetReason() == WifiTxTimer::WAIT_REGRANT_AFTER_TXS))
        {
            WifiTxVector ackTxVector =
                GetWifiRemoteStationManager()->GetAckTxVector(hdr.GetAddr2(), txVector);

            Time ackTxDuration =
                m_phy->GetSifs() +
                m_phy->CalculateTxDuration(GetAckSize(), ackTxVector, m_phy->GetPhyBand());

            m_txTimer.Cancel();
            if (GetRemainingTxsDuration() > ackTxDuration + m_phy->GetPifs())
            {
                ResetTxTimer(mpdu, txVector, sender, ackTxDuration + m_phy->GetPifs());
            }
            else
            {
                ResetTxTimer(mpdu, txVector, sender, ackTxDuration + m_phy->GetSifs());
            }
        }

        HeFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
        return;
    }

    if (hdr.IsMgt())
    {
        Mac48Address sender = *(m_txTimer.GetStasExpectedToRespond().begin());
        if (m_txTimer.IsRunning() && (hdr.GetAddr1() == m_self) && (hdr.GetAddr2() == sender) &&
            (m_txTimer.GetReason() == WifiTxTimer::WAIT_REGRANT_AFTER_TXS))
        {
            WifiTxVector ackTxVector =
                GetWifiRemoteStationManager()->GetAckTxVector(hdr.GetAddr2(), txVector);

            Time ackTxDuration =
                m_phy->GetSifs() +
                m_phy->CalculateTxDuration(GetAckSize(), ackTxVector, m_phy->GetPhyBand());

            m_txTimer.Cancel();
            if (GetRemainingTxsDuration() > ackTxDuration + m_phy->GetPifs())
            {
                ResetTxTimer(mpdu, txVector, sender, ackTxDuration + m_phy->GetPifs());
            }
            else
            {
                ResetTxTimer(mpdu, txVector, sender, ackTxDuration + m_phy->GetSifs());
            }
        }

        HeFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
        return;
    }

    if (m_txTimer.IsRunning() && (hdr.GetAddr1() == m_self) &&
        (m_txTimer.GetReason() == WifiTxTimer::WAIT_REGRANT_AFTER_TXS))
    {
        NS_LOG_ERROR("Not implemented");
    }

    if (hdr.IsQosData() && hdr.GetAddr1().IsGroup())
    {
        NS_LOG_ERROR("Not implemented");
    }

    EhtFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
}

void
TxsFrameExchangeManager::EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                                         const RxSignalInfo& rxSignalInfo,
                                         const WifiTxVector& txVector,
                                         const std::vector<bool>& perMpduStatus)
{
    Mac48Address sender = *(m_txTimer.GetStasExpectedToRespond().begin());

    if (m_txTimer.IsRunning() && (psdu->GetAddr1() == m_self) && (psdu->GetAddr2() == sender) &&
        (m_txTimer.GetReason() == WifiTxTimer::WAIT_REGRANT_AFTER_TXS))
    {
        std::set<uint8_t> tids = psdu->GetTids();

        // Multi-TID A-MPDUs are not supported yet
        if (tids.size() == 1)
        {
            NS_ASSERT(psdu->GetNMpdus() > 1);
            double rxSnr = rxSignalInfo.snr;
            uint8_t tid = *tids.begin();
            WifiMacHeader::QosAckPolicy ackPolicy = psdu->GetAckPolicyForTid(tid);

            if (ackPolicy == WifiMacHeader::NORMAL_ACK)
            {
                // Normal Ack or Implicit Block Ack Request
                auto agreement = m_mac->GetBaAgreementEstablishedAsRecipient(psdu->GetAddr2(), tid);

                auto blockAckTxVector =
                    GetWifiRemoteStationManager()->GetBlockAckTxVector(psdu->GetAddr2(), txVector);

                Time blockAckTxDuration =
                    GetBlockAckDuration(*agreement, psdu->GetDuration(), blockAckTxVector, rxSnr);

                m_txTimer.Cancel();
                if (GetRemainingTxsDuration() > blockAckTxDuration + m_phy->GetPifs())
                {
                    ResetTxTimer(Create<WifiMpdu>(psdu->GetPayload(0), psdu->GetHeader(0)),
                                 blockAckTxVector,
                                 sender,
                                 blockAckTxDuration + m_phy->GetPifs());
                }
                else
                {
                    ResetTxTimer(Create<WifiMpdu>(psdu->GetPayload(0), psdu->GetHeader(0)),
                                 blockAckTxVector,
                                 sender,
                                 blockAckTxDuration + m_phy->GetSifs());
                }
            }
        }
        HtFrameExchangeManager::EndReceiveAmpdu(psdu, rxSignalInfo, txVector, perMpduStatus);
        return;
    }
    HeFrameExchangeManager::EndReceiveAmpdu(psdu, rxSignalInfo, txVector, perMpduStatus);
}

void
TxsFrameExchangeManager::ResetTxTimer(Ptr<const WifiMpdu> mpdu,
                                      const WifiTxVector& txVector,
                                      const Mac48Address& macAddress,
                                      Time duration)
{
    Ptr<Packet> dummyPacket = Create<Packet>();
    Ptr<WifiMpdu> dummyMpdu = Create<WifiMpdu>(dummyPacket, mpdu->GetHeader());

    m_txTimer.Set(WifiTxTimer::WAIT_REGRANT_AFTER_TXS,
                  duration,
                  {macAddress},
                  &TxsFrameExchangeManager::CheckReGrantConditions,
                  this,
                  dummyMpdu,
                  txVector);
}

Time
TxsFrameExchangeManager::GetBlockAckDuration(const RecipientBlockAckAgreement& agreement,
                                             Time durationId,
                                             WifiTxVector& blockAckTxVector,
                                             double rxSnr)
{
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_BACKRESP);

    CtrlBAckResponseHeader blockAck;
    blockAck.SetType(agreement.GetBlockAckType());

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(blockAck);
    Ptr<WifiPsdu> psdu = GetWifiPsdu(Create<WifiMpdu>(packet, hdr), blockAckTxVector);
    return m_phy->GetSifs() +
           m_phy->CalculateTxDuration(psdu, blockAckTxVector, m_phy->GetPhyBand());
}

void
TxsFrameExchangeManager::SendBlockAck(const RecipientBlockAckAgreement& agreement,
                                      Time durationId,
                                      WifiTxVector& blockAckTxVector,
                                      double rxSnr)
{
    HtFrameExchangeManager::SendBlockAck(agreement, durationId, blockAckTxVector, rxSnr);
}

void
TxsFrameExchangeManager::SendNormalAck(const WifiMacHeader& hdr,
                                       const WifiTxVector& dataTxVector,
                                       double dataSnr)
{
    HtFrameExchangeManager::SendNormalAck(hdr, dataTxVector, dataSnr);
}

void
TxsFrameExchangeManager::SendCtsAfterMuRtsTxs(const WifiMacHeader& muRtsHdr,
                                              const TxsCtrlTriggerHeader& trigger,
                                              double muRtsSnr)
{
    NS_LOG_FUNCTION(this << muRtsHdr << trigger << muRtsSnr);
    const auto userInfoIt = trigger.FindUserInfoWithAid(m_staMac->GetAssociationId());
    WifiTxVector ctsTxVector;
    uint16_t allowedWidth;
    if (trigger.GetCsRequired())
    {
        if (m_navEnd > Simulator::Now())
        {
            NS_LOG_INFO("Basic NAV indicates medium busy");
            return;
        }

        std::set<uint8_t> indices;
        ctsTxVector = GetCtsTxVectorAfterMuRts(trigger, m_staMac->GetAssociationId());
        allowedWidth = ctsTxVector.GetChannelWidth();
        indices = m_phy->GetOperatingChannel().GetAll20MHzChannelIndicesInPrimary(allowedWidth);
        if (m_channelAccessManager->GetPer20MHzBusy(indices))
        {
            NS_LOG_INFO("CCA BUSY");
            return;
        }
    }

    protectedFromMuRtsTxs = true;
    ctsTxVector.SetTriggerResponding(
        true); // to create the identical UID with MU-RTS TXS trigger frame

    Time allocatedTxopDuration = userInfoIt->GetAllocationDuration();

    // m_edca = m_mac->GetQosTxop(muRtsHdr.GetQosTid());

    WifiMacHeader qosHdr = muRtsHdr;
    qosHdr.SetType(WIFI_MAC_QOSDATA);
    m_edca = m_mac->GetQosTxop(qosHdr.GetQosTid());
    m_edca->NotifyChannelAccessed(m_linkId, allocatedTxopDuration - MicroSeconds(16));

    // 이전에 선언되면, CS결과가 busy일때, channel release를 다시 호출해야함.

    NS_LOG_INFO("Allowed Time: " << m_edca->GetRemainingTxop(m_linkId).GetMicroSeconds());

    Time nextTxInterval =
        m_phy->GetSifs() +
        m_phy->CalculateTxDuration(GetCtsSize(), ctsTxVector, m_phy->GetPhyBand());

    // NS_LOG_INFO("Next TX time: " << nextTxInterval.GetMicroSeconds());

    Simulator::Schedule(nextTxInterval,
                        &TxsFrameExchangeManager::StartTransmission,
                        this,
                        m_edca,
                        allowedWidth);

    NS_LOG_INFO("Shared STA (" << m_self << ") sends CTS to AP (" << muRtsHdr.GetAddr2()
                               << ") that sent MU-RTS TXS");
    // NS_LOG_INFO("Shared STA (" << m_self << ") will send non-TB PPDU to the AP ("
    //                            << muRtsHdr.GetAddr2() << ") After SIFS + CTS duration");

    DoSendCtsAfterRts(muRtsHdr, ctsTxVector, muRtsSnr);

    // m_edca->NotifyChannelAccessed(m_linkId);
}

bool
TxsFrameExchangeManager::SendMuRtsTxs(const WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this);
    return false;
}

void
TxsFrameExchangeManager::SetTxsParams(Time txsDuration)
{
    m_txsParams.txsDuration = txsDuration;
    m_txsParams.txsStart = Simulator::Now();
}

TxsParams
TxsFrameExchangeManager::GetTxsParams()
{
    return m_txsParams;
}

Time
TxsFrameExchangeManager::GetRemainingTxsDuration() const
{
    Time elapsedTime = Simulator::Now() - m_txsParams.txsStart;
    Time remainingTxsDuration = m_txsParams.txsDuration - elapsedTime;
    if (remainingTxsDuration.IsStrictlyNegative())
    {
        remainingTxsDuration = Seconds(0);
    }
    return remainingTxsDuration;
}

WifiTxVector
TxsFrameExchangeManager::GetCtsTxVectorAfterMuRts(const TxsCtrlTriggerHeader& trigger,
                                                  uint16_t staId) const
{
    NS_LOG_FUNCTION(this << trigger << staId);

    auto userInfoIt = trigger.FindUserInfoWithAid(staId);
    NS_ASSERT_MSG(userInfoIt != trigger.end(), "User Info field for AID=" << staId << " not found");
    uint16_t bw = 0;

    if (uint8_t ru = userInfoIt->GetMuRtsRuAllocation(); ru < 65)
    {
        bw = 20;
    }
    else if (ru < 67)
    {
        bw = 40;
    }
    else if (ru == 67)
    {
        bw = 80;
    }
    else
    {
        NS_ASSERT(ru == 68);
        bw = 160;
    }

    auto txVector = GetWifiRemoteStationManager()->GetCtsTxVector(m_bssid, GetCtsModeAfterMuRts());
    // set the channel width of the CTS TXVECTOR according to the allocated RU
    txVector.SetChannelWidth(bw);

    return txVector;
}

bool
TxsFrameExchangeManager::StartTransmission(Ptr<QosTxop> edca, uint16_t allowedWidth)
{
    if (m_navEnd > Simulator::Now())
    {
        NS_LOG_INFO("Basic NAV indicates medium busy");
        return false;
    }

    m_allowedWidth = allowedWidth;

    Ptr<WifiMpdu> peekedItem = edca->PeekNextMpdu(m_linkId);
    if (!peekedItem || (peekedItem->GetHeader().GetAddr1() != m_txopHolder))
    {
        NS_LOG_INFO("Shared STA no has frame to be transmitted to AP");
        NotifyChannelReleased(m_edca);
        m_edca = nullptr;
        return false;
    }
    else
    {
        NS_LOG_INFO("Shared STA sends non-TB PPDU to AP");
        m_initialFrame = false;
        return QosFrameExchangeManager::StartTransmission(edca, Seconds(0));
    }
}

void
TxsFrameExchangeManager::CtsAfterMuRtsTxsTimeout(Ptr<WifiMpdu> muRts, const WifiTxVector& txVector)
{
    NS_LOG_INFO("Current Time when running CtsTimeout: " << Simulator::Now().GetMicroSeconds());
    HeFrameExchangeManager::CtsAfterMuRtsTimeout(muRts, txVector);
}

void
TxsFrameExchangeManager::CheckReGrantConditions(Ptr<const WifiMpdu> mpdu,
                                                const WifiTxVector& txVector)
{
    // TODO : check the results of CS is idle
    if (m_edca->GetRemainingTxop(m_linkId).IsNegative())
    {
        // Remaining TXOP time is not enough
        NotifyChannelReleased(m_edca);
        m_edca = nullptr;
        return;
    }

    Time lastBusy = m_channelAccessManager->GetAccessGrantStart(false) - m_phy->GetSifs();

    if (lastBusy > Simulator::Now() || m_navEnd > Simulator::Now())
    {
        // Medium is not Idle at PIFS boundary
        if (m_txTimer.IsRunning())
        {
            m_txTimer.Cancel();
        }

        // NS_LOG_INFO("End of TXOP: "
        //             << (m_edca->GetRemainingTxop(m_linkId) +
        //             Simulator::Now()).GetMicroSeconds());

        Time lastBusyEnd = std::max(lastBusy, m_navEnd);

        NS_LOG_INFO("LastBusyEnd: " << lastBusyEnd.GetMicroSeconds());
        if (lastBusyEnd > (m_edca->GetRemainingTxop(m_linkId) + Simulator::Now()))
        {
            NS_LOG_INFO("AP channel release because remaining TXOP is not enough");
            NotifyChannelReleased(m_edca);
            m_edca = nullptr;
            return;
        }

        Mac48Address sharedStaAddress = *(m_txTimer.GetStasExpectedToRespond().begin());
        ResetTxTimer(mpdu,
                     txVector,
                     sharedStaAddress,
                     lastBusyEnd - Simulator::Now() + m_phy->GetPifs());
    }
    else
    {
        // Medium is Idle at PIFS boundary
        // QosFrameExchangeManager::StartTransmission(m_edca, Seconds(0));
        NS_LOG_INFO("AP regrants TXOP and releases channel");
        // NotifyChannelReleased(m_edca);
        // m_edca = nullptr;
        QosFrameExchangeManager::SendCfEndIfNeeded();
    }
}

void
TxsFrameExchangeManager::PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    auto txVectorCopy = txVector;

    if (psdu->GetNMpdus() == 1 && psdu->GetHeader(0).IsTrigger())
    {
        TxsCtrlTriggerHeader commonInfoField(false);
        psdu->GetPayload(0)->PeekHeader(commonInfoField);
        if (commonInfoField.IsMuRts() &&
            ((commonInfoField.GetTxsMode() == TxsModes::MU_RTS_TXS_MODE_1) ||
             (commonInfoField.GetTxsMode() == TxsModes::MU_RTS_TXS_MODE_2)))
        {
            TxsCtrlTriggerHeader trigger;
            psdu->GetPayload(0)->PeekHeader(trigger);
            const WifiMacHeader& muRts = psdu->GetHeader(0);

            WifiMacHeader rts;
            rts.SetType(WIFI_MAC_CTL_RTS);
            rts.SetDsFrom(); // MU-RTS TXS only
            rts.SetDsNotTo();
            rts.SetDuration(muRts.GetDuration());
            rts.SetAddr2(muRts.GetAddr2());
            if (m_staMac != nullptr && m_staMac->IsAssociated() &&
                muRts.GetAddr2() == m_bssid // sent by the AP this STA is associated with
                && trigger.FindUserInfoWithAid(m_staMac->GetAssociationId()) != trigger.end())
            {
                // the MU-RTS is addressed to this station
                rts.SetAddr1(m_self);
            }
            else
            {
                rts.SetAddr1(muRts.GetAddr2()); // an address different from that of this station
            }
            psdu = Create<const WifiPsdu>(Create<Packet>(), rts);
            // The duration of the NAV reset timeout has to take into account that the CTS
            // response is sent using the 6 Mbps data rate
            txVectorCopy =
                GetWifiRemoteStationManager()->GetCtsTxVector(m_bssid, GetCtsModeAfterMuRts());
            HeFrameExchangeManager::PostProcessFrame(psdu, txVectorCopy);
        }
        else
        {
            HeFrameExchangeManager::PostProcessFrame(psdu, txVectorCopy);
        }
    }
    VhtFrameExchangeManager::PostProcessFrame(psdu, txVectorCopy);
}

void
TxsFrameExchangeManager::TransmissionFailed()
{
    if (protectedFromMuRtsTxs == true)
    {
        m_initialFrame = true; // If the transmission of shared STA operting TXS mode 1 fails, it
                               // releases the channel.
        protectedFromMuRtsTxs = false;
    }
    EhtFrameExchangeManager::TransmissionFailed();
}

void
TxsFrameExchangeManager::NotifyChannelReleased(Ptr<Txop> txop)
{
    protectedFromMuRtsTxs = false;
    EhtFrameExchangeManager::NotifyChannelReleased(txop);
}

void
TxsFrameExchangeManager::SetSharedStaAddress(Mac48Address macAddress)
{
    m_sharedStaAddress = macAddress;
}

Mac48Address
TxsFrameExchangeManager::GetSharedStaAddress() const
{
    return m_sharedStaAddress;
}

uint8_t
TxsFrameExchangeManager::GetTidFromAc(AcIndex ac)
{
    switch (ac)
    {
    case AC_BE:
        return 0;

    case AC_BK:
        return 1;

    case AC_VI:
        return 4;

    case AC_VO:
        return 6;
    default:
        NS_ABORT_MSG("QoS service must be supported in TxsFunction");
        return 8;
    }
}

} // namespace ns3
