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

#include "txs-ctrl-headers.h"

#include "ns3/address-utils.h"
#include "ns3/ctrl-headers.h"
#include "ns3/he-phy.h"
#include "ns3/log.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-utils.h"

#include <algorithm>

#define TEST_MODE 1

namespace ns3
{
#if TEST_MODE == 1

NS_LOG_COMPONENT_DEFINE("TxsCtrlTriggerHeader");

TxsCtrlTriggerUserInfoField::TxsCtrlTriggerUserInfoField(TriggerFrameType triggerType,
                                                         TriggerFrameVariant variant)
    : m_variant(variant),
      m_aid12(0),
      m_ruAllocation(0),
      m_ps160(false),
      m_triggerType(triggerType)
{
    // memset(&m_bits26To31, 0, sizeof(m_bits26To31));
}

TxsCtrlTriggerUserInfoField::~TxsCtrlTriggerUserInfoField()
{
}

TxsCtrlTriggerUserInfoField&
TxsCtrlTriggerUserInfoField::operator=(const TxsCtrlTriggerUserInfoField& userInfo)
{
    NS_ABORT_MSG_IF(m_triggerType != userInfo.m_triggerType, "Trigger Frame type mismatch");

    // check for self-assignment
    if (&userInfo == this)
    {
        return *this;
    }

    m_variant = userInfo.m_variant;
    m_aid12 = userInfo.m_aid12;
    m_ruAllocation = userInfo.m_ruAllocation;
    m_ps160 = userInfo.m_ps160;
    return *this;
}

void
TxsCtrlTriggerUserInfoField::Print(std::ostream& os) const
{
    os << ", USER_INFO " << (m_variant == TriggerFrameVariant::HE ? "HE" : "EHT")
       << " variant AID=" << m_aid12 << ", RU_Allocation=" << +m_ruAllocation;
}

uint32_t
TxsCtrlTriggerUserInfoField::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 5; // User Info (excluding Trigger Dependent User Info)

    NS_ABORT_MSG_IF(m_triggerType != TriggerFrameType::MU_RTS_TRIGGER,
                    "Trigger frame type is not MU-RTS");

    return size;
}

Buffer::Iterator
TxsCtrlTriggerUserInfoField::Serialize(Buffer::Iterator start) const
{
    NS_ABORT_MSG_IF(m_triggerType != TriggerFrameType::MU_RTS_TRIGGER,
                    "Trigger frame type is not MU-RTS");

    Buffer::Iterator i = start;

    uint32_t userInfo = 0; // User Info except the MSB
    userInfo |= (m_aid12 & 0x0fff);
    userInfo |= (m_ruAllocation << 12);
    userInfo |= (m_allocationDuration << 20);

    i.WriteHtolsbU32(userInfo);
    // Here we need to write 8 bits covering the UL Target RSSI (7 bits) and B39, which is
    // reserved in the HE variant and the PS160 subfield in the EHT variant.
    uint8_t bit32To39 = 0;
    if (m_variant == TriggerFrameVariant::EHT)
    {
        bit32To39 |= (m_ps160 ? 1 << 7 : 0);
    }

    i.WriteU8(bit32To39);

    return i;
}

Buffer::Iterator
TxsCtrlTriggerUserInfoField::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    uint32_t userInfo = i.ReadLsbtohU32();

    m_aid12 = userInfo & 0x0fff;
    NS_ABORT_MSG_IF(m_aid12 == 4095, "Cannot deserialize a Padding field");
    m_ruAllocation = (userInfo >> 12) & 0xff;
    m_allocationDuration = (userInfo >> 20) & 0x01ff;

    NS_ABORT_MSG_IF(!(0 < m_aid12 && m_aid12 < 2045), "AID12 must be on the range 1 ~ 2044");

    uint8_t bit32To39 = i.ReadU8();
    if (m_variant == TriggerFrameVariant::EHT)
    {
        m_ps160 = (bit32To39 >> 7) == 1;
    }

    return i;
}

TriggerFrameType
TxsCtrlTriggerUserInfoField::GetType() const
{
    return m_triggerType;
}

WifiPreamble
TxsCtrlTriggerUserInfoField::GetPreambleType() const
{
    switch (m_variant)
    {
    case TriggerFrameVariant::HE:
        return WIFI_PREAMBLE_HE_TB;
    case TriggerFrameVariant::EHT:
        return WIFI_PREAMBLE_EHT_TB;
    default:
        NS_ABORT_MSG("Unexpected variant: " << +static_cast<uint8_t>(m_variant));
    }
    return WIFI_PREAMBLE_LONG; // to silence warning
}

void
TxsCtrlTriggerUserInfoField::SetAid12(uint16_t aid)
{
    m_aid12 = aid & 0x0fff;
}

uint16_t
TxsCtrlTriggerUserInfoField::GetAid12() const
{
    return m_aid12;
}

void
TxsCtrlTriggerUserInfoField::SetMuRtsRuAllocation(uint8_t value)
{
    NS_ABORT_MSG_IF(m_triggerType != TriggerFrameType::MU_RTS_TRIGGER,
                    "SetMuRtsRuAllocation() can only be used for MU-RTS");
    NS_ABORT_MSG_IF(
        value < 61 || value > 68,
        "Value "
            << +value
            << " is not admitted for B7-B1 of the RU Allocation subfield of MU-RTS Trigger Frames");

    m_ruAllocation = (value << 1);
    if (value == 68)
    {
        // set B0 for 160 MHz and 80+80 MHz indication
        m_ruAllocation++;
    }
}

uint8_t
TxsCtrlTriggerUserInfoField::GetMuRtsRuAllocation() const
{
    NS_ABORT_MSG_IF(m_triggerType != TriggerFrameType::MU_RTS_TRIGGER,
                    "GetMuRtsRuAllocation() can only be used for MU-RTS");
    uint8_t value = (m_ruAllocation >> 1);
    NS_ABORT_MSG_IF(
        value < 61 || value > 68,
        "Value "
            << +value
            << " is not admitted for B7-B1 of the RU Allocation subfield of MU-RTS Trigger Frames");
    return value;
}

void
TxsCtrlTriggerUserInfoField::SetAllocationDuration(Time allowedTxopTime)
{
    NS_LOG_FUNCTION(this << allowedTxopTime);
    uint16_t allowedTxopTime16MicroUnit = allowedTxopTime.GetMicroSeconds() / 16;
    NS_LOG_INFO("Allowing time: " << allowedTxopTime.GetMicroSeconds());

    // NS_ABORT_MSG_IF(allowedTxopTime.GetMicroSeconds() % 16 != 0,
    //                 "allowedTxopTime16MicroUnit is not 16 MicroSec Unit");
    m_allocationDuration = allowedTxopTime16MicroUnit;
}

Time
TxsCtrlTriggerUserInfoField::GetAllocationDuration() const
{
    return MicroSeconds(m_allocationDuration) * 16;
}

/***********************************
 *       Trigger frame
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED(TxsCtrlTriggerHeader);

TxsCtrlTriggerHeader::TxsCtrlTriggerHeader()
    : m_variant(TriggerFrameVariant::HE),
      m_triggerType(TriggerFrameType::BASIC_TRIGGER),
      m_ulLength(0),
      m_moreTF(false),
      m_csRequired(false),
      m_ulBandwidth(0),
      m_TxsMode(0),
      m_apTxPower(0),
      m_ulSpatialReuse(0),
      m_padding(0),
      m_doUiDeserialization(true)
{
}

TxsCtrlTriggerHeader::TxsCtrlTriggerHeader(bool m_doUiDeserialization)
    : m_variant(TriggerFrameVariant::HE),
      m_triggerType(TriggerFrameType::BASIC_TRIGGER),
      m_ulLength(0),
      m_moreTF(false),
      m_csRequired(false),
      m_ulBandwidth(0),
      m_TxsMode(0),
      m_apTxPower(0),
      m_ulSpatialReuse(0),
      m_padding(0),
      m_doUiDeserialization(m_doUiDeserialization)
{
}

TxsCtrlTriggerHeader::~TxsCtrlTriggerHeader()
{
}

TxsCtrlTriggerHeader&
TxsCtrlTriggerHeader::operator=(const TxsCtrlTriggerHeader& trigger)
{
    // check for self-assignment
    if (&trigger == this)
    {
        return *this;
    }

    m_variant = trigger.m_variant;
    m_triggerType = trigger.m_triggerType;
    m_ulLength = trigger.m_ulLength;
    m_moreTF = trigger.m_moreTF;
    m_csRequired = trigger.m_csRequired;
    m_ulBandwidth = trigger.m_ulBandwidth;
    m_TxsMode = trigger.m_TxsMode;
    m_apTxPower = trigger.m_apTxPower;
    m_ulSpatialReuse = trigger.m_ulSpatialReuse;
    m_padding = trigger.m_padding;
    m_userInfoFields.clear();
    m_userInfoFields = trigger.m_userInfoFields;
    return *this;
}

TypeId
TxsCtrlTriggerHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TxsCtrlTriggerHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<TxsCtrlTriggerHeader>();
    return tid;
}

TypeId
TxsCtrlTriggerHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
TxsCtrlTriggerHeader::Print(std::ostream& os) const
{
    os << "TriggerType=" << GetTypeString() << ", Bandwidth=" << +GetUlBandwidth()
       << ", UL Length=" << m_ulLength;

    for (auto& ui : m_userInfoFields)
    {
        ui.Print(os);
    }
}

void
TxsCtrlTriggerHeader::SetVariant(TriggerFrameVariant variant)
{
    NS_ABORT_MSG_IF(!m_userInfoFields.empty(),
                    "Cannot change Common Info field variant if User Info fields are present");
    m_variant = variant;
}

TriggerFrameVariant
TxsCtrlTriggerHeader::GetVariant() const
{
    return m_variant;
}

uint32_t
TxsCtrlTriggerHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 8; // Common Info (excluding Trigger Dependent Common Info)

    NS_ABORT_MSG_IF(m_triggerType != TriggerFrameType::MU_RTS_TRIGGER,
                    "Trigger frame type is not MU-RTS");

    for (auto& ui : m_userInfoFields)
    {
        size += ui.GetSerializedSize();
    }

    size += m_padding;

    return size;
}

void
TxsCtrlTriggerHeader::Serialize(Buffer::Iterator start) const
{
    NS_ABORT_MSG_IF(m_triggerType != TriggerFrameType::MU_RTS_TRIGGER,
                    "Trigger frame type is not MU-RTS");

    Buffer::Iterator i = start;

    uint64_t commonInfo = 0;
    commonInfo |= (static_cast<uint8_t>(m_triggerType) & 0x0f);
    commonInfo |= (m_ulLength & 0x0fff) << 4;
    commonInfo |= (m_moreTF ? 1 << 16 : 0);
    commonInfo |= (m_csRequired ? 1 << 17 : 0);
    commonInfo |= (m_ulBandwidth & 0x03) << 18;
    commonInfo |= (m_TxsMode & 0x03) << 20;
    commonInfo |= static_cast<uint64_t>(m_apTxPower & 0x3f) << 28;
    commonInfo |= static_cast<uint64_t>(m_ulSpatialReuse) << 37;
    if (m_variant == TriggerFrameVariant::HE)
    {
        uint64_t ulHeSigA2 = 0x01ff; // nine bits equal to 1
        commonInfo |= ulHeSigA2 << 54;
    }

    i.WriteHtolsbU64(commonInfo);

    for (auto& ui : m_userInfoFields)
    {
        i = ui.Serialize(i);
    }

    for (std::size_t count = 0; count < m_padding; count++)
    {
        i.WriteU8(0xff); // Padding field
    }
}

uint32_t
TxsCtrlTriggerHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    uint64_t commonInfo = i.ReadLsbtohU64();

    m_triggerType = static_cast<TriggerFrameType>(commonInfo & 0x0f);
    m_ulLength = (commonInfo >> 4) & 0x0fff;
    m_moreTF = (commonInfo >> 16) & 0x01;
    m_csRequired = (commonInfo >> 17) & 0x01;
    m_ulBandwidth = (commonInfo >> 18) & 0x03;
    m_TxsMode = (commonInfo >> 20) & 0x03;
    m_apTxPower = (commonInfo >> 28) & 0x3f;
    m_ulSpatialReuse = (commonInfo >> 37) & 0xffff;
    uint8_t bit54and55 = (commonInfo >> 54) & 0x03;
    m_variant = bit54and55 == 3 ? TriggerFrameVariant::HE : TriggerFrameVariant::EHT;
    m_userInfoFields.clear();
    m_padding = 0;

    if (m_doUiDeserialization)
    {
        while (i.GetRemainingSize() >= 2)
        {
            // read the first 2 bytes to check if we encountered the Padding field
            if (i.ReadU16() == 0xffff)
            {
                m_padding = i.GetRemainingSize() + 2;
            }
            else
            {
                // go back 2 bytes to deserialize the User Info field from the beginning
                i.Prev(2);
                TxsCtrlTriggerUserInfoField& ui = AddUserInfoField();
                i = ui.Deserialize(i);
            }
        }
    }
    return i.GetDistanceFrom(start);
}

void
TxsCtrlTriggerHeader::SetType(TriggerFrameType type)
{
    m_triggerType = type;
}

TriggerFrameType
TxsCtrlTriggerHeader::GetType() const
{
    return m_triggerType;
}

const char*
TxsCtrlTriggerHeader::GetTypeString() const
{
    return GetTypeString(GetType());
}

const char*
TxsCtrlTriggerHeader::GetTypeString(TriggerFrameType type)
{
#define FOO(x)                                                                                     \
    case TriggerFrameType::x:                                                                      \
        return #x;

    switch (type)
    {
        FOO(BASIC_TRIGGER);
        FOO(BFRP_TRIGGER);
        FOO(MU_BAR_TRIGGER);
        FOO(MU_RTS_TRIGGER);
        FOO(BSRP_TRIGGER);
        FOO(GCR_MU_BAR_TRIGGER);
        FOO(BQRP_TRIGGER);
        FOO(NFRP_TRIGGER);
    default:
        return "ERROR";
    }
#undef FOO
}

bool
TxsCtrlTriggerHeader::IsBasic() const
{
    return (m_triggerType == TriggerFrameType::BASIC_TRIGGER);
}

bool
TxsCtrlTriggerHeader::IsBfrp() const
{
    return (m_triggerType == TriggerFrameType::BFRP_TRIGGER);
}

bool
TxsCtrlTriggerHeader::IsMuBar() const
{
    return (m_triggerType == TriggerFrameType::MU_BAR_TRIGGER);
}

bool
TxsCtrlTriggerHeader::IsMuRts() const
{
    return (m_triggerType == TriggerFrameType::MU_RTS_TRIGGER);
}

bool
TxsCtrlTriggerHeader::IsBsrp() const
{
    return (m_triggerType == TriggerFrameType::BSRP_TRIGGER);
}

bool
TxsCtrlTriggerHeader::IsGcrMuBar() const
{
    return (m_triggerType == TriggerFrameType::GCR_MU_BAR_TRIGGER);
}

bool
TxsCtrlTriggerHeader::IsBqrp() const
{
    return (m_triggerType == TriggerFrameType::BQRP_TRIGGER);
}

bool
TxsCtrlTriggerHeader::IsNfrp() const
{
    return (m_triggerType == TriggerFrameType::NFRP_TRIGGER);
}

void
TxsCtrlTriggerHeader::SetUlLength(uint16_t len)
{
    m_ulLength = (len & 0x0fff);
}

uint16_t
TxsCtrlTriggerHeader::GetUlLength() const
{
    return m_ulLength;
}

void
TxsCtrlTriggerHeader::SetMoreTF(bool more)
{
    m_moreTF = more;
}

bool
TxsCtrlTriggerHeader::GetMoreTF() const
{
    return m_moreTF;
}

void
TxsCtrlTriggerHeader::SetCsRequired(bool cs)
{
    m_csRequired = cs;
}

bool
TxsCtrlTriggerHeader::GetCsRequired() const
{
    return m_csRequired;
}

void
TxsCtrlTriggerHeader::SetUlBandwidth(uint16_t bw)
{
    switch (bw)
    {
    case 20:
        m_ulBandwidth = 0;
        break;
    case 40:
        m_ulBandwidth = 1;
        break;
    case 80:
        m_ulBandwidth = 2;
        break;
    case 160:
        m_ulBandwidth = 3;
        break;
    default:
        NS_FATAL_ERROR("Bandwidth value not allowed.");
        break;
    }
}

uint16_t
TxsCtrlTriggerHeader::GetUlBandwidth() const
{
    return (1 << m_ulBandwidth) * 20;
}

void
TxsCtrlTriggerHeader::SetTxsMode(TxsModes mode)
{
    m_TxsMode = mode;
}

uint8_t
TxsCtrlTriggerHeader::GetTxsMode() const
{
    return m_TxsMode;
}

void
TxsCtrlTriggerHeader::SetApTxPower(int8_t power)
{
    // see Table 9-25f "AP Tx Power subfield encoding" of 802.11ax amendment D3.0
    NS_ABORT_MSG_IF(power < -20 || power > 40, "Out of range power values");

    m_apTxPower = static_cast<uint8_t>(power + 20);
}

int8_t
TxsCtrlTriggerHeader::GetApTxPower() const
{
    // see Table 9-25f "AP Tx Power subfield encoding" of 802.11ax amendment D3.0
    return static_cast<int8_t>(m_apTxPower) - 20;
}

void
TxsCtrlTriggerHeader::SetUlSpatialReuse(uint16_t sr)
{
    m_ulSpatialReuse = sr;
}

uint16_t
TxsCtrlTriggerHeader::GetUlSpatialReuse() const
{
    return m_ulSpatialReuse;
}

void
TxsCtrlTriggerHeader::SetPaddingSize(std::size_t size)
{
    NS_ABORT_MSG_IF(size == 1, "The Padding field, if present, shall be at least two octets");
    m_padding = size;
}

std::size_t
TxsCtrlTriggerHeader::GetPaddingSize() const
{
    return m_padding;
}

TxsCtrlTriggerHeader
TxsCtrlTriggerHeader::GetCommonInfoField() const
{
    // make a copy of this Trigger Frame and remove the User Info fields from the copy
    TxsCtrlTriggerHeader trigger(*this);
    trigger.m_userInfoFields.clear();
    return trigger;
}

TxsCtrlTriggerUserInfoField&
TxsCtrlTriggerHeader::AddUserInfoField()
{
    m_userInfoFields.emplace_back(m_triggerType, m_variant);
    return m_userInfoFields.back();
}

void
TxsCtrlTriggerHeader::AddUserInfoToMuRts(const Mac48Address& receiver,
                                         Ptr<ApWifiMac> apMac,
                                         uint8_t linkId,
                                         Ptr<WifiRemoteStationManager> WifiRemoteStationManager)
{
    NS_LOG_FUNCTION(this << GetUlBandwidth() << receiver);

    TxsCtrlTriggerUserInfoField& ui = AddUserInfoField();

    NS_ABORT_MSG_IF(apMac->GetTypeOfStation() != AP, "HE APs only can send MU-RTS");
    ui.SetAid12(apMac->GetAssociationId(receiver, linkId));

    const uint16_t ctsTxWidth =
        std::min((u_int16_t)GetUlBandwidth(),
                 WifiRemoteStationManager->GetChannelWidthSupported(receiver));
    auto phy = apMac->GetWifiPhy(linkId);
    std::size_t primaryIdx = phy->GetOperatingChannel().GetPrimaryChannelIndex(ctsTxWidth);
    if (phy->GetChannelWidth() == 160 && ctsTxWidth <= 40 && primaryIdx >= 80 / ctsTxWidth)
    {
        // the primary80 is in the higher part of the 160 MHz channel
        primaryIdx -= 80 / ctsTxWidth;
    }
    switch (ctsTxWidth)
    {
    case 20:
        ui.SetMuRtsRuAllocation(61 + primaryIdx);
        break;
    case 40:
        ui.SetMuRtsRuAllocation(65 + primaryIdx);
        break;
    case 80:
        ui.SetMuRtsRuAllocation(67);
        break;
    case 160:
        ui.SetMuRtsRuAllocation(68);
        break;
    default:
        NS_ABORT_MSG("Unhandled TX width: " << ctsTxWidth << " MHz");
    }
}

TxsCtrlTriggerHeader::ConstIterator
TxsCtrlTriggerHeader::begin() const
{
    return m_userInfoFields.begin();
}

TxsCtrlTriggerHeader::ConstIterator
TxsCtrlTriggerHeader::end() const
{
    return m_userInfoFields.end();
}

TxsCtrlTriggerHeader::Iterator
TxsCtrlTriggerHeader::begin()
{
    return m_userInfoFields.begin();
}

TxsCtrlTriggerHeader::Iterator
TxsCtrlTriggerHeader::end()
{
    return m_userInfoFields.end();
}

std::size_t
TxsCtrlTriggerHeader::GetNUserInfoFields() const
{
    return m_userInfoFields.size();
}

TxsCtrlTriggerHeader::ConstIterator
TxsCtrlTriggerHeader::FindUserInfoWithAid(ConstIterator start, uint16_t aid12) const
{
    // the lambda function returns true if a User Info field has the AID12 subfield
    // equal to the given aid12 value
    return std::find_if(start, end(), [aid12](const TxsCtrlTriggerUserInfoField& ui) -> bool {
        return (ui.GetAid12() == aid12);
    });
}

TxsCtrlTriggerHeader::ConstIterator
TxsCtrlTriggerHeader::FindUserInfoWithAid(uint16_t aid12) const
{
    return FindUserInfoWithAid(m_userInfoFields.begin(), aid12);
}

TxsCtrlTriggerHeader::ConstIterator
TxsCtrlTriggerHeader::FindUserInfoWithRaRuAssociated(ConstIterator start) const
{
    return FindUserInfoWithAid(start, 0);
}

TxsCtrlTriggerHeader::ConstIterator
TxsCtrlTriggerHeader::FindUserInfoWithRaRuAssociated() const
{
    return FindUserInfoWithAid(0);
}

TxsCtrlTriggerHeader::ConstIterator
TxsCtrlTriggerHeader::FindUserInfoWithRaRuUnassociated(ConstIterator start) const
{
    return FindUserInfoWithAid(start, 2045);
}

TxsCtrlTriggerHeader::ConstIterator
TxsCtrlTriggerHeader::FindUserInfoWithRaRuUnassociated() const
{
    return FindUserInfoWithAid(2045);
}

bool
TxsCtrlTriggerHeader::IsValid() const
{
    if (m_triggerType == TriggerFrameType::MU_RTS_TRIGGER)
    {
        return true;
    }

    // check that allocated RUs do not overlap
    // TODO This is not a problem in case of UL MU-MIMO
    NS_ABORT_MSG_IF(m_userInfoFields.size() != 1,
                    "The MU-RTS trigger frame has only one User Infomation Field.");
    return false;
}

#endif

} // namespace ns3
