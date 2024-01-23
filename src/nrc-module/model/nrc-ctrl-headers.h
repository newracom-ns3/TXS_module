#ifndef NRC_CTRL_HEADERS_H
#define NRC_CTRL_HEADERS_H

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

class NrcCtrlTriggerUserInfoField
{
  public:
    NrcCtrlTriggerUserInfoField(TriggerFrameType triggerType, TriggerFrameVariant variant);
    ~NrcCtrlTriggerUserInfoField();
    NrcCtrlTriggerUserInfoField& operator=(const NrcCtrlTriggerUserInfoField& userInfo);

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

class NrcCtrlTriggerHeader : public Header
{
  public:
    NrcCtrlTriggerHeader();
    NrcCtrlTriggerHeader(bool m_doUiDeserialization);
    ~NrcCtrlTriggerHeader() override;

    NrcCtrlTriggerHeader& operator=(const NrcCtrlTriggerHeader& trigger);

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

    NrcCtrlTriggerHeader GetCommonInfoField() const;

    // undefiend yet
    NrcCtrlTriggerUserInfoField& AddUserInfoField();
    void AddUserInfoToMuRts(const Mac48Address& receiver,
                            Ptr<ApWifiMac> apMac,
                            uint8_t linkId,
                            Ptr<WifiRemoteStationManager> WifiRemoteStationManager);

    NrcCtrlTriggerUserInfoField GetUserInfoField();

    /// User Info fields list const iterator
    typedef std::list<NrcCtrlTriggerUserInfoField>::const_iterator ConstIterator;
    /// User Info fields list iterator
    typedef std::list<NrcCtrlTriggerUserInfoField>::iterator Iterator;

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
    std::list<NrcCtrlTriggerUserInfoField> m_userInfoFields; //!< list of User Info fields
};

#endif

} // namespace ns3

#endif /* CTRL_HEADERS_H */
