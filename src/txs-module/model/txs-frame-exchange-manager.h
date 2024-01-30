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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Contributed by Stefano Avallone <stavallo@unina.it>
 *
 * This code has been based on ns-3 (wifi/model/eht/eht-frame-exchange-manager.{cc,h})
 * Author: Seungmin Lee <sm.lee@newratek.com>
 *         Changmin Lee <cm.lee@newratek.com>
 */

#ifndef TXS_FRAME_EXCHANGE_MANAGER_H
#define TXS_FRAME_EXCHANGE_MANAGER_H

#include "txs-ctrl-headers.h"

#include "ns3/eht-frame-exchange-manager.h"

// Add a doxygen group for this module.
// If you have more than one file, this should be in only one of them.
/**
 * \defgroup txs-module Description of the txs-module
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

class TxsFrameExchangeManager : public EhtFrameExchangeManager
{
  public:
    static TypeId GetTypeId();
    TxsFrameExchangeManager();
    ~TxsFrameExchangeManager() override;

  protected:
    void DoDispose() override;

    bool TrySendMuRtsTxs(const Time availableTime,
                         const Time txDuration,
                         const WifiTxVector& ctsTxVector) const;

    bool SendMuRtsTxs(const WifiTxParameters& txParams);

    bool StartFrameExchange(Ptr<QosTxop> edca, Time availableTime, bool initialFrame) override;
    bool SendMuRtsTxs(const Mac48Address& receiver, const Time availableTime);
    WifiTxVector GetCtsTxVectorAfterMuRts(const TxsCtrlTriggerHeader& trigger,
                                          uint16_t staId) const;
    void CtsAfterMuRtsTxsTimeout(Ptr<WifiMpdu> muRts, const WifiTxVector& txVector);
    void CheckReGrantConditions(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector);

    Time CalculateMuRtsTxDuration(const TxsCtrlTriggerHeader& muRtsTxs,
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
                              const TxsCtrlTriggerHeader& trigger,
                              double muRtsSnr);
    bool IsInValid(Ptr<const WifiMpdu> mpdu) const;
    void SetImaginaryPsdu();
    bool StartTransmissionInTxs(Ptr<QosTxop> edca, uint16_t allowedWidth);
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
    void PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) override;
    void TransmissionFailed() override;
    void NotifyChannelReleased(Ptr<Txop> txop) override;

  private:
    WifiTxParameters m_txParams; //!< the TX parameters for the MU-RTX TXS frame;
    Mac48Address m_sharedStaAddress;
    TxsParams m_txsParams;
    TracedValue<uint32_t> txsCount;
    uint8_t GetTidFromAc(AcIndex ac);
};

// Each class should be documented using Doxygen,
// and have an \ingroup txs-module directive

/* ... */

} // namespace ns3

#endif /* TXS_FRAME_EXCHANGE_MANAGER_H */
