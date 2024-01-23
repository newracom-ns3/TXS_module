#include "nrc-ctrl-headers.h"

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

NS_LOG_COMPONENT_DEFINE("NrcCtrlTriggerHeader");

NrcCtrlTriggerUserInfoField::NrcCtrlTriggerUserInfoField(TriggerFrameType triggerType,
                                                         TriggerFrameVariant variant)
    : m_variant(variant),
      m_aid12(0),
      m_ruAllocation(0),
      m_ps160(false),
      m_triggerType(triggerType)
{
    // memset(&m_bits26To31, 0, sizeof(m_bits26To31));
}

NrcCtrlTriggerUserInfoField::~NrcCtrlTriggerUserInfoField()
{
}

NrcCtrlTriggerUserInfoField&
NrcCtrlTriggerUserInfoField::operator=(const NrcCtrlTriggerUserInfoField& userInfo)
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
NrcCtrlTriggerUserInfoField::Print(std::ostream& os) const
{
    os << ", USER_INFO " << (m_variant == TriggerFrameVariant::HE ? "HE" : "EHT")
       << " variant AID=" << m_aid12 << ", RU_Allocation=" << +m_ruAllocation;
}

uint32_t
NrcCtrlTriggerUserInfoField::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 5; // User Info (excluding Trigger Dependent User Info)

    NS_ABORT_MSG_IF(m_triggerType != TriggerFrameType::MU_RTS_TRIGGER,
                    "Trigger frame type is not MU-RTS");

    return size;
}

Buffer::Iterator
NrcCtrlTriggerUserInfoField::Serialize(Buffer::Iterator start) const
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
NrcCtrlTriggerUserInfoField::Deserialize(Buffer::Iterator start)
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
NrcCtrlTriggerUserInfoField::GetType() const
{
    return m_triggerType;
}

WifiPreamble
NrcCtrlTriggerUserInfoField::GetPreambleType() const
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
NrcCtrlTriggerUserInfoField::SetAid12(uint16_t aid)
{
    m_aid12 = aid & 0x0fff;
}

uint16_t
NrcCtrlTriggerUserInfoField::GetAid12() const
{
    return m_aid12;
}

void
NrcCtrlTriggerUserInfoField::SetMuRtsRuAllocation(uint8_t value)
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
NrcCtrlTriggerUserInfoField::GetMuRtsRuAllocation() const
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
NrcCtrlTriggerUserInfoField::SetAllocationDuration(Time allowedTxopTime)
{
    NS_LOG_FUNCTION(this << allowedTxopTime);
    uint16_t allowedTxopTime16MicroUnit = allowedTxopTime.GetMicroSeconds() / 16;
    NS_LOG_INFO("Allowing time: " << allowedTxopTime.GetMicroSeconds());

    // NS_ABORT_MSG_IF(allowedTxopTime.GetMicroSeconds() % 16 != 0,
    //                 "allowedTxopTime16MicroUnit is not 16 MicroSec Unit");
    m_allocationDuration = allowedTxopTime16MicroUnit;
}

Time
NrcCtrlTriggerUserInfoField::GetAllocationDuration() const
{
    return MicroSeconds(m_allocationDuration) * 16;
}

/***********************************
 *       Trigger frame
 ***********************************/

NS_OBJECT_ENSURE_REGISTERED(NrcCtrlTriggerHeader);

NrcCtrlTriggerHeader::NrcCtrlTriggerHeader()
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

NrcCtrlTriggerHeader::NrcCtrlTriggerHeader(bool m_doUiDeserialization)
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

NrcCtrlTriggerHeader::~NrcCtrlTriggerHeader()
{
}

NrcCtrlTriggerHeader&
NrcCtrlTriggerHeader::operator=(const NrcCtrlTriggerHeader& trigger)
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
NrcCtrlTriggerHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NrcCtrlTriggerHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<NrcCtrlTriggerHeader>();
    return tid;
}

TypeId
NrcCtrlTriggerHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
NrcCtrlTriggerHeader::Print(std::ostream& os) const
{
    os << "TriggerType=" << GetTypeString() << ", Bandwidth=" << +GetUlBandwidth()
       << ", UL Length=" << m_ulLength;

    for (auto& ui : m_userInfoFields)
    {
        ui.Print(os);
    }
}

void
NrcCtrlTriggerHeader::SetVariant(TriggerFrameVariant variant)
{
    NS_ABORT_MSG_IF(!m_userInfoFields.empty(),
                    "Cannot change Common Info field variant if User Info fields are present");
    m_variant = variant;
}

TriggerFrameVariant
NrcCtrlTriggerHeader::GetVariant() const
{
    return m_variant;
}

uint32_t
NrcCtrlTriggerHeader::GetSerializedSize() const
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
NrcCtrlTriggerHeader::Serialize(Buffer::Iterator start) const
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
NrcCtrlTriggerHeader::Deserialize(Buffer::Iterator start)
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
                NrcCtrlTriggerUserInfoField& ui = AddUserInfoField();
                i = ui.Deserialize(i);
            }
        }
    }
    return i.GetDistanceFrom(start);
}

void
NrcCtrlTriggerHeader::SetType(TriggerFrameType type)
{
    m_triggerType = type;
}

TriggerFrameType
NrcCtrlTriggerHeader::GetType() const
{
    return m_triggerType;
}

const char*
NrcCtrlTriggerHeader::GetTypeString() const
{
    return GetTypeString(GetType());
}

const char*
NrcCtrlTriggerHeader::GetTypeString(TriggerFrameType type)
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
NrcCtrlTriggerHeader::IsBasic() const
{
    return (m_triggerType == TriggerFrameType::BASIC_TRIGGER);
}

bool
NrcCtrlTriggerHeader::IsBfrp() const
{
    return (m_triggerType == TriggerFrameType::BFRP_TRIGGER);
}

bool
NrcCtrlTriggerHeader::IsMuBar() const
{
    return (m_triggerType == TriggerFrameType::MU_BAR_TRIGGER);
}

bool
NrcCtrlTriggerHeader::IsMuRts() const
{
    return (m_triggerType == TriggerFrameType::MU_RTS_TRIGGER);
}

bool
NrcCtrlTriggerHeader::IsBsrp() const
{
    return (m_triggerType == TriggerFrameType::BSRP_TRIGGER);
}

bool
NrcCtrlTriggerHeader::IsGcrMuBar() const
{
    return (m_triggerType == TriggerFrameType::GCR_MU_BAR_TRIGGER);
}

bool
NrcCtrlTriggerHeader::IsBqrp() const
{
    return (m_triggerType == TriggerFrameType::BQRP_TRIGGER);
}

bool
NrcCtrlTriggerHeader::IsNfrp() const
{
    return (m_triggerType == TriggerFrameType::NFRP_TRIGGER);
}

void
NrcCtrlTriggerHeader::SetUlLength(uint16_t len)
{
    m_ulLength = (len & 0x0fff);
}

uint16_t
NrcCtrlTriggerHeader::GetUlLength() const
{
    return m_ulLength;
}

void
NrcCtrlTriggerHeader::SetMoreTF(bool more)
{
    m_moreTF = more;
}

bool
NrcCtrlTriggerHeader::GetMoreTF() const
{
    return m_moreTF;
}

void
NrcCtrlTriggerHeader::SetCsRequired(bool cs)
{
    m_csRequired = cs;
}

bool
NrcCtrlTriggerHeader::GetCsRequired() const
{
    return m_csRequired;
}

void
NrcCtrlTriggerHeader::SetUlBandwidth(uint16_t bw)
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
NrcCtrlTriggerHeader::GetUlBandwidth() const
{
    return (1 << m_ulBandwidth) * 20;
}

void
NrcCtrlTriggerHeader::SetTxsMode(TxsModes mode)
{
    m_TxsMode = mode;
}

uint8_t
NrcCtrlTriggerHeader::GetTxsMode() const
{
    return m_TxsMode;
}

void
NrcCtrlTriggerHeader::SetApTxPower(int8_t power)
{
    // see Table 9-25f "AP Tx Power subfield encoding" of 802.11ax amendment D3.0
    NS_ABORT_MSG_IF(power < -20 || power > 40, "Out of range power values");

    m_apTxPower = static_cast<uint8_t>(power + 20);
}

int8_t
NrcCtrlTriggerHeader::GetApTxPower() const
{
    // see Table 9-25f "AP Tx Power subfield encoding" of 802.11ax amendment D3.0
    return static_cast<int8_t>(m_apTxPower) - 20;
}

void
NrcCtrlTriggerHeader::SetUlSpatialReuse(uint16_t sr)
{
    m_ulSpatialReuse = sr;
}

uint16_t
NrcCtrlTriggerHeader::GetUlSpatialReuse() const
{
    return m_ulSpatialReuse;
}

void
NrcCtrlTriggerHeader::SetPaddingSize(std::size_t size)
{
    NS_ABORT_MSG_IF(size == 1, "The Padding field, if present, shall be at least two octets");
    m_padding = size;
}

std::size_t
NrcCtrlTriggerHeader::GetPaddingSize() const
{
    return m_padding;
}

NrcCtrlTriggerHeader
NrcCtrlTriggerHeader::GetCommonInfoField() const
{
    // make a copy of this Trigger Frame and remove the User Info fields from the copy
    NrcCtrlTriggerHeader trigger(*this);
    trigger.m_userInfoFields.clear();
    return trigger;
}

NrcCtrlTriggerUserInfoField&
NrcCtrlTriggerHeader::AddUserInfoField()
{
    m_userInfoFields.emplace_back(m_triggerType, m_variant);
    return m_userInfoFields.back();
}

void
NrcCtrlTriggerHeader::AddUserInfoToMuRts(const Mac48Address& receiver,
                                         Ptr<ApWifiMac> apMac,
                                         uint8_t linkId,
                                         Ptr<WifiRemoteStationManager> WifiRemoteStationManager)
{
    NS_LOG_FUNCTION(this << GetUlBandwidth() << receiver);

    NrcCtrlTriggerUserInfoField& ui = AddUserInfoField();

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

NrcCtrlTriggerHeader::ConstIterator
NrcCtrlTriggerHeader::begin() const
{
    return m_userInfoFields.begin();
}

NrcCtrlTriggerHeader::ConstIterator
NrcCtrlTriggerHeader::end() const
{
    return m_userInfoFields.end();
}

NrcCtrlTriggerHeader::Iterator
NrcCtrlTriggerHeader::begin()
{
    return m_userInfoFields.begin();
}

NrcCtrlTriggerHeader::Iterator
NrcCtrlTriggerHeader::end()
{
    return m_userInfoFields.end();
}

std::size_t
NrcCtrlTriggerHeader::GetNUserInfoFields() const
{
    return m_userInfoFields.size();
}

NrcCtrlTriggerHeader::ConstIterator
NrcCtrlTriggerHeader::FindUserInfoWithAid(ConstIterator start, uint16_t aid12) const
{
    // the lambda function returns true if a User Info field has the AID12 subfield
    // equal to the given aid12 value
    return std::find_if(start, end(), [aid12](const NrcCtrlTriggerUserInfoField& ui) -> bool {
        return (ui.GetAid12() == aid12);
    });
}

NrcCtrlTriggerHeader::ConstIterator
NrcCtrlTriggerHeader::FindUserInfoWithAid(uint16_t aid12) const
{
    return FindUserInfoWithAid(m_userInfoFields.begin(), aid12);
}

NrcCtrlTriggerHeader::ConstIterator
NrcCtrlTriggerHeader::FindUserInfoWithRaRuAssociated(ConstIterator start) const
{
    return FindUserInfoWithAid(start, 0);
}

NrcCtrlTriggerHeader::ConstIterator
NrcCtrlTriggerHeader::FindUserInfoWithRaRuAssociated() const
{
    return FindUserInfoWithAid(0);
}

NrcCtrlTriggerHeader::ConstIterator
NrcCtrlTriggerHeader::FindUserInfoWithRaRuUnassociated(ConstIterator start) const
{
    return FindUserInfoWithAid(start, 2045);
}

NrcCtrlTriggerHeader::ConstIterator
NrcCtrlTriggerHeader::FindUserInfoWithRaRuUnassociated() const
{
    return FindUserInfoWithAid(2045);
}

bool
NrcCtrlTriggerHeader::IsValid() const
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
