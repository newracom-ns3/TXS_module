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
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Contributed by Mirko Banchi <mk.banchi@gmail.com>
 *
 * This code has been based on ns-3 (wifi/model/ctrl-header.{cc,h})
 * Author: Seungmin Lee <sm.lee@newratek.com>
 *         Changmin Lee <cm.lee@newratek.com>
 */

#ifndef TXS_CTRL_HEADERS_H
#define TXS_CTRL_HEADERS_H

#include "ns3/ap-wifi-mac.h"
#include "ns3/block-ack-type.h"
#include "ns3/he-ru.h"
// #include "ns3/header.h"
#include "ns3/ctrl-headers.h"
#include "ns3/mac48-address.h"
#include "ns3/wifi-phy-common.h"

#include <list>
#include <vector>

#define TEST_MODE 1

namespace ns3
{
#if TEST_MODE == 1

class WifiTxVector;
enum AcIndex : uint8_t;

enum class TriggerFrameType : uint8_t;
enum class TriggerFrameVariant : uint8_t;

enum TxsModes : uint8_t
{
    MU_RTS_MODE = 0,
    MU_RTS_TXS_MODE_1 = 1,
    MU_RTS_TXS_MODE_2 = 2,
    INVALID = 3
};

class TxsCtrlTriggerUserInfoField
{
  public:
    TxsCtrlTriggerUserInfoField(TriggerFrameType triggerType, TriggerFrameVariant variant);
    ~TxsCtrlTriggerUserInfoField();
    TxsCtrlTriggerUserInfoField& operator=(const TxsCtrlTriggerUserInfoField& userInfo);

    void Print(std::ostream& os) const;

    uint32_t GetSerializedSize() const;

    Buffer::Iterator Serialize(Buffer::Iterator start) const;

    Buffer::Iterator Deserialize(Buffer::Iterator start);

    TriggerFrameType GetType() const;

    WifiPreamble GetPreambleType() const;

    void SetAid12(uint16_t aid);

    uint16_t GetAid12() const;

    /**
     * Set the RU Allocation subfield based on the given value for the B7-B1 bits.
     * This method can only be called on MU-RTS Trigger Frames.
     *
     * B7–B1 of the RU Allocation subfield is set to indicate the primary 20 MHz channel
     * as follows:
     * - 61 if the primary 20 MHz channel is the only 20 MHz channel or the lowest frequency
     *   20 MHz channel in the primary 40 MHz channel or primary 80 MHz channel
     * - 62 if the primary 20 MHz channel is the second lowest frequency 20 MHz channel in the
     *   primary 40 MHz channel or primary 80 MHz channel
     * - 63 if the primary 20 MHz channel is the third lowest frequency 20 MHz channel in the
     *   primary 80 MHz channel
     * - 64 if the primary 20 MHz channel is the fourth lowest frequency 20 MHz channel in the
     *   primary 80 MHz channel
     *
     * B7–B1 of the RU Allocation subfield is set to indicate the primary 40 MHz channel
     * as follows:
     * - 65 if the primary 40 MHz channel is the only 40 MHz channel or the lowest frequency
     *   40 MHz channel in the primary 80 MHz channel
     * - 66 if the primary 40 MHz channel is the second lowest frequency 40 MHz channel in the
     *   primary 80 MHz channel
     *
     * B7–B1 of the RU Allocation subfield is set to 67 to indicate the primary 80 MHz channel.
     *
     * B7–B1 of the RU Allocation subfield is set to 68 to indicate the primary and secondary
     * 80 MHz channel.
     *
     * \param value the value for B7–B1 of the RU Allocation subfield
     */
    void SetMuRtsRuAllocation(uint8_t value);

    uint8_t GetMuRtsRuAllocation() const;

    void SetAllocationDuration(Time allowedTxopTime);
    Time GetAllocationDuration() const;

  private:
    TriggerFrameVariant m_variant; //!< User Info field variant

    uint16_t m_aid12;       //!< Association ID of the addressed station
    uint8_t m_ruAllocation; //!< RU Allocation
    uint16_t m_allocationDuration;
    bool m_ps160; //!< identifies the location of the RU (EHT variant only

    TriggerFrameType m_triggerType; //!< Trigger frame type
};

class TxsCtrlTriggerHeader : public Header
{
  public:
    TxsCtrlTriggerHeader();
    TxsCtrlTriggerHeader(bool m_doUiDeserialization);
    ~TxsCtrlTriggerHeader() override;

    TxsCtrlTriggerHeader& operator=(const TxsCtrlTriggerHeader& trigger);

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    void SetVariant(TriggerFrameVariant variant);

    TriggerFrameVariant GetVariant() const;

    void SetType(TriggerFrameType type);

    TriggerFrameType GetType() const;

    const char* GetTypeString() const;

    static const char* GetTypeString(TriggerFrameType type);

    bool IsBasic() const;

    bool IsBfrp() const;

    bool IsMuBar() const;

    bool IsMuRts() const;

    bool IsBsrp() const;

    bool IsGcrMuBar() const;

    bool IsBqrp() const;

    bool IsNfrp() const;

    void SetUlLength(uint16_t len);

    uint16_t GetUlLength() const;

    void SetMoreTF(bool more);

    bool GetMoreTF() const;

    void SetCsRequired(bool cs);

    bool GetCsRequired() const;

    void SetUlBandwidth(uint16_t bw);

    uint16_t GetUlBandwidth() const;

    void SetTxsMode(TxsModes mode);

    uint8_t GetTxsMode() const;

    void SetApTxPower(int8_t power);

    int8_t GetApTxPower() const;

    void SetUlSpatialReuse(uint16_t sr);

    uint16_t GetUlSpatialReuse() const;

    void SetPaddingSize(std::size_t size);

    std::size_t GetPaddingSize() const;

    TxsCtrlTriggerHeader GetCommonInfoField() const;

    // undefiend yet
    TxsCtrlTriggerUserInfoField& AddUserInfoField();
    void AddUserInfoToMuRts(const Mac48Address& receiver,
                            Ptr<ApWifiMac> apMac,
                            uint8_t linkId,
                            Ptr<WifiRemoteStationManager> WifiRemoteStationManager);

    TxsCtrlTriggerUserInfoField GetUserInfoField();

    /// User Info fields list const iterator
    typedef std::list<TxsCtrlTriggerUserInfoField>::const_iterator ConstIterator;
    /// User Info fields list iterator
    typedef std::list<TxsCtrlTriggerUserInfoField>::iterator Iterator;

    ConstIterator begin() const;
    ConstIterator end() const;
    Iterator begin();
    Iterator end();
    std::size_t GetNUserInfoFields() const;
    ConstIterator FindUserInfoWithAid(ConstIterator start, uint16_t aid12) const;
    ConstIterator FindUserInfoWithAid(uint16_t aid12) const;
    ConstIterator FindUserInfoWithRaRuAssociated(ConstIterator start) const;
    ConstIterator FindUserInfoWithRaRuAssociated() const;
    ConstIterator FindUserInfoWithRaRuUnassociated(ConstIterator start) const;
    ConstIterator FindUserInfoWithRaRuUnassociated() const;

    bool IsValid() const;

  private:
    /**
     * Common Info field
     */
    TriggerFrameVariant m_variant;  //!< Common Info field variant
    TriggerFrameType m_triggerType; //!< Trigger type
    uint16_t m_ulLength;            //!< Value for the L-SIG Length field
    bool m_moreTF;                  //!< True if a subsequent Trigger frame follows
    bool m_csRequired;              //!< Carrier Sense required
    uint8_t m_ulBandwidth;          //!< UL BW subfield
    uint8_t m_TxsMode;              //!< GI And LTF Type subfield
    uint8_t m_apTxPower;            //!< Tx Power used by AP to transmit the Trigger Frame
    uint16_t m_ulSpatialReuse;      //!< Value for the Spatial Reuse field in HE-SIG-A
    std::size_t m_padding;          //!< the size in bytes of the Padding field
    bool m_doUiDeserialization;
    /**
     * List of User Info fields
     */
    std::list<TxsCtrlTriggerUserInfoField> m_userInfoFields; //!< list of User Info fields
};

#endif

} // namespace ns3

#endif /* CTRL_HEADERS_H */
