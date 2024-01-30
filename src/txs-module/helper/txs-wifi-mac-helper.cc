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
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2016
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
 * Contributed by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *                Sébastien Deronne <sebastien.deronne@gmail.com>
 *
 * This code has been based on ns-3 (wifi/model/wifi-mac.{cc,h})
 *                                  (wifi/model/wifi-mac-helper.{cc,h})
 *
 * Author: Seungmin Lee <sm.lee@newratek.com>
 *         Changmin Lee <cm.lee@newratek.com>
 */

#include "txs-wifi-mac-helper.h"

#include "ns3/boolean.h"
#include "ns3/channel-access-manager.h"
#include "ns3/eht-configuration.h"
#include "ns3/eht-frame-exchange-manager.h"
#include "ns3/emlsr-manager.h"
#include "ns3/extended-capabilities.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/he-configuration.h"
#include "ns3/ht-configuration.h"
#include "ns3/log.h"
#include "ns3/mac-rx-middle.h"
#include "ns3/mac-tx-middle.h"
#include "ns3/mgt-headers.h"
#include "ns3/multi-user-scheduler.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/ssid.h"
#include "ns3/txs-frame-exchange-manager.h"
#include "ns3/vht-configuration.h"
#include "ns3/wifi-ack-manager.h"
#include "ns3/wifi-assoc-manager.h"
#include "ns3/wifi-mac-queue-scheduler.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-protection-manager.h"

namespace ns3
{

TxsWifiMac::TxsWifiMac()
{
}

TxsWifiMac::~TxsWifiMac()
{
}

void
TxsWifiMac::FemConfigureStandard(WifiStandard standard, bool txsSupported)
{
    for (uint8_t id = 0; id < GetNLinks(); id++)
    {
        auto& link = GetLink(id);
        SetupFrameExchangeManager(standard, link, txsSupported);
    }
}

void
TxsWifiMac::SetupFrameExchangeManager(WifiStandard standard,
                                      WifiMac::LinkEntity& link,
                                      bool txsSupported)
{
    NS_ABORT_MSG_IF(standard == WIFI_STANDARD_UNSPECIFIED, "Wifi standard not set");

    if (standard >= WIFI_STANDARD_80211be)
    {
        // This sniff is edited by @sm.lee from the code of Mathieu Lacage who associated INRIA
        if (txsSupported)
        {
            link.feManager = CreateObject<TxsFrameExchangeManager>();
        }
        else
        {
            link.feManager = CreateObject<EhtFrameExchangeManager>();
        }
    }
    else if (standard >= WIFI_STANDARD_80211ax)
    {
        link.feManager = CreateObject<HeFrameExchangeManager>();
    }
    else if (standard >= WIFI_STANDARD_80211ac)
    {
        link.feManager = CreateObject<VhtFrameExchangeManager>();
    }
    else if (standard >= WIFI_STANDARD_80211n)
    {
        link.feManager = CreateObject<HtFrameExchangeManager>();
    }
    else if (GetQosSupported())
    {
        link.feManager = CreateObject<QosFrameExchangeManager>();
    }
    else
    {
        link.feManager = CreateObject<FrameExchangeManager>();
    }

    link.feManager->SetMacTxMiddle(m_txMiddle);
    link.feManager->SetMacRxMiddle(m_rxMiddle);
    link.feManager->SetAddress(GetAddress());

    link.feManager->GetWifiTxTimer().SetMpduResponseTimeoutCallback(
        MakeCallback(&MpduResponseTimeoutTracedCallback::operator(),
                     &m_mpduResponseTimeoutCallback));
    link.feManager->GetWifiTxTimer().SetPsduResponseTimeoutCallback(
        MakeCallback(&PsduResponseTimeoutTracedCallback::operator(),
                     &m_psduResponseTimeoutCallback));
    link.feManager->GetWifiTxTimer().SetPsduMapResponseTimeoutCallback(
        MakeCallback(&PsduMapResponseTimeoutTracedCallback::operator(),
                     &m_psduMapResponseTimeoutCallback));
    link.feManager->SetDroppedMpduCallback(
        MakeCallback(&DroppedMpduTracedCallback::operator(), &m_droppedMpduCallback));
    link.feManager->SetAckedMpduCallback(
        MakeCallback(&MpduTracedCallback::operator(), &m_ackedMpduCallback));
    link.feManager->GetWifiTxTimer().SetMpduResponseTimeoutCallback(
        MakeCallback(&MpduResponseTimeoutTracedCallback::operator(),
                     &m_mpduResponseTimeoutCallback));
    link.feManager->GetWifiTxTimer().SetPsduResponseTimeoutCallback(
        MakeCallback(&PsduResponseTimeoutTracedCallback::operator(),
                     &m_psduResponseTimeoutCallback));
    link.feManager->GetWifiTxTimer().SetPsduMapResponseTimeoutCallback(
        MakeCallback(&PsduMapResponseTimeoutTracedCallback::operator(),
                     &m_psduMapResponseTimeoutCallback));
    link.feManager->SetDroppedMpduCallback(
        MakeCallback(&DroppedMpduTracedCallback::operator(), &m_droppedMpduCallback));
    link.feManager->SetAckedMpduCallback(
        MakeCallback(&MpduTracedCallback::operator(), &m_ackedMpduCallback));
}

TxsWifiMacHelper::TxsWifiMacHelper(bool enable)
    : m_txsSupported(enable)
{
}

TxsWifiMacHelper::~TxsWifiMacHelper()
{
}

Ptr<WifiMac>
TxsWifiMacHelper::Create(Ptr<WifiNetDevice> device, WifiStandard standard) const
{
    NS_ABORT_MSG_IF(standard == WIFI_STANDARD_UNSPECIFIED, "No standard specified!");

    // this is a const method, but we need to force the correct QoS setting
    ObjectFactory macObjectFactory = m_mac;
    if (standard >= WIFI_STANDARD_80211n)
    {
        macObjectFactory.Set("QosSupported", BooleanValue(true));
    }

    Ptr<WifiMac> mac = macObjectFactory.Create<WifiMac>();

    mac->SetDevice(device);
    mac->SetAddress(Mac48Address::Allocate());
    device->SetMac(mac);

    // This sniff is edited by @sm.lee from the code of Sébastien Deronne
    Ptr<TxsWifiMac> txsMac = StaticCast<TxsWifiMac>(mac);
    txsMac->FemConfigureStandard(standard, m_txsSupported);
    //
    mac->ConfigureStandard(standard);

    Ptr<WifiMacQueueScheduler> queueScheduler = m_queueScheduler.Create<WifiMacQueueScheduler>();
    mac->SetMacQueueScheduler(queueScheduler);

    // WaveNetDevice (through ns-3.38) stores PHY entities in a different member than
    // WifiNetDevice, hence GetNPhys() would return 0. We have to attach a protection manager
    // and an ack manager to the unique instance of frame exchange manager anyway
    for (uint8_t linkId = 0; linkId < std::max<uint8_t>(device->GetNPhys(), 1); ++linkId)
    {
        auto fem = mac->GetFrameExchangeManager(linkId);

        Ptr<WifiProtectionManager> protectionManager =
            m_protectionManager.Create<WifiProtectionManager>();
        protectionManager->SetWifiMac(mac);
        protectionManager->SetLinkId(linkId);
        fem->SetProtectionManager(protectionManager);

        Ptr<WifiAckManager> ackManager = m_ackManager.Create<WifiAckManager>();
        ackManager->SetWifiMac(mac);
        ackManager->SetLinkId(linkId);
        fem->SetAckManager(ackManager);

        // 11be MLDs require a MAC address to be assigned to each STA. Note that
        // FrameExchangeManager objects are created by WifiMac::SetupFrameExchangeManager
        // (which is invoked by WifiMac::ConfigureStandard, which is called above),
        // which sets the FrameExchangeManager's address to the address held by WifiMac.
        // Hence, in case the number of PHY objects is 1, the FrameExchangeManager's
        // address equals the WifiMac's address.
        if (device->GetNPhys() > 1)
        {
            fem->SetAddress(Mac48Address::Allocate());
        }
    }

    // create and install the Multi User Scheduler if this is an HE AP
    Ptr<ApWifiMac> apMac;
    if (standard >= WIFI_STANDARD_80211ax && m_muScheduler.IsTypeIdSet() &&
        (apMac = DynamicCast<ApWifiMac>(mac)))
    {
        Ptr<MultiUserScheduler> muScheduler = m_muScheduler.Create<MultiUserScheduler>();
        apMac->AggregateObject(muScheduler);
    }

    // create and install the Association Manager if this is a STA
    auto staMac = DynamicCast<StaWifiMac>(mac);
    if (staMac)
    {
        Ptr<WifiAssocManager> assocManager = m_assocManager.Create<WifiAssocManager>();
        staMac->SetAssocManager(assocManager);
    }

    // create and install the EMLSR Manager if this is an EHT non-AP MLD with EMLSR activated
    if (BooleanValue emlsrActivated;
        standard >= WIFI_STANDARD_80211be && staMac && staMac->GetNLinks() > 1 &&
        device->GetEhtConfiguration()->GetAttributeFailSafe("EmlsrActivated", emlsrActivated) &&
        emlsrActivated.Get())
    {
        auto emlsrManager = m_emlsrManager.Create<EmlsrManager>();
        staMac->SetEmlsrManager(emlsrManager);
    }

    return mac;
}

} // namespace ns3