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

 * Copyright (c) 2016 SEBASTIEN DERONNE
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
 * Author: Sebastien Deronne <sebastien.deronne@gmail.com>
 *
 * This code has been based on ns-3 (examples/wireless/wifi-he-network.cc)
 * Author: Changmin Lee <cm.lee@newratek.com>
 *         Seungmin Lee <sm.lee@newratek.com>
 */

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/eht-configuration.h"
#include "ns3/enum.h"
#include "ns3/he-phy.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/stats-module.h"
#include "ns3/string.h"
#include "ns3/txs-wifi-mac-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-mac-queue.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("mu-ul-example");

void
SpectrumNetworkInstallation(WifiMacHelper& mac,
                            SpectrumWifiPhyHelper& phy,
                            Ssid& ssid,
                            WifiHelper& wifi,
                            std::string& channelStr,
                            NodeContainer& wifiStaNodes,
                            NodeContainer& wifiApNode,
                            NetDeviceContainer& staDevices,
                            NetDeviceContainer& apDevice,
                            std::string& dlAckSeqType,
                            bool& enableUlOfdma,
                            bool& enableBsrp,
                            Time& accessReqInterval)
{
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    phy.Set("ChannelSettings", StringValue(channelStr));
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    if (dlAckSeqType != "NO-OFDMA")
    {
        mac.SetMultiUserScheduler("ns3::RrMultiUserScheduler",
                                  "EnableUlOfdma",
                                  BooleanValue(enableUlOfdma),
                                  "EnableBsrp",
                                  BooleanValue(enableBsrp),
                                  "AccessReqInterval",
                                  TimeValue(accessReqInterval));
    }
    mac.SetType("ns3::ApWifiMac",
                "QosSupported",
                BooleanValue(true),
                "EnableBeaconJitter",
                BooleanValue(false),
                "Ssid",
                SsidValue(ssid));
    apDevice = wifi.Install(phy, mac, wifiApNode);
};

void
InstallMobility(double distanceBtw,
                double distance,
                NodeContainer& wifiApNode,
                NodeContainer& wifiStaNodes,
                double BSSIndex)
{
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0 + (distanceBtw * BSSIndex), 0.0, 0.0));
    positionAlloc->Add(Vector(distance + (distanceBtw * BSSIndex), 0.0, 0.0));
    positionAlloc->Add(Vector(-distance + (distanceBtw * BSSIndex), 0.0, 0.0));
    positionAlloc->Add(Vector(0.0 + (distanceBtw * BSSIndex), distance, 0.0));
    positionAlloc->Add(Vector(0.0 + (distanceBtw * BSSIndex), -distance, 0.0));

    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);
};

enum accessCategoryIndex : uint8_t
{
    BE = 0,
    BK,
    VI,
    VO
};

int
main(int argc, char* argv[])
{
    // QoS
    accessCategoryIndex packetAc{VI}; // BE, BK, VI, VO
    bool packetQosSupport{true};

    // PHY
    std::string phyModel{"Spectrum"};
    bool fixedPhyBand{true};
    if (fixedPhyBand == true)
        Config::SetDefault("ns3::WifiPhy::FixedPhyBand", BooleanValue(true));
    int channelIndex = 42;
    int primaryChannelIndex = 3;
    double frequency{5};       // whether 2.4, 5 or 6 GHz
    uint8_t channelWidth = 80; // 80 MHz
    int gi{800};               // Guard interval = 3200, 1600, 800 [ms]
    int mcs{5};                // -1 indicates an unset value

    // MAC
    bool useRts{true};
    bool useExtendedBlockAck{false};
    Time accessReqInterval{0};         // AP MAC
    std::string macQueueSize = "100p"; // Max size of MAC queue (Packet unit)
    Config::SetDefault("ns3::WifiMacQueue::MaxSize", QueueSizeValue(QueueSize(macQueueSize)));

    // Mobility
    double distanceBtw{10.0};
    double distance{1.0}; // meters

    // Network setting
    bool isEhtNetwork{false}; // True: EHT network, False: HE network
    bool txsEnabled{false};   // True: TXS supported
    bool udp{true};
    bool enableUlOfdma{true}; // True: mu-ul
    bool enableBsrp{false};
    std::size_t nStations{4}; // The number of STAs
    std::string dlAckSeqType{"MU-BAR"};
    uint32_t payloadSize = 700;    // Bytes
    int applicationDataRate = 100; // Application Data rate (Mbit/s)
    Config::SetDefault("ns3::RrMultiUserScheduler::NStations",
                       UintegerValue(3)); // Control the number of STA to participate MU operation

    // Simulation time
    double simulationTime{4}; // Seconds

    if (useRts)
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("0"));
        Config::SetDefault("ns3::WifiDefaultProtectionManager::EnableMuRts", BooleanValue(true));
    }

    if (dlAckSeqType == "ACK-SU-FORMAT")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE));
    }
    else if (dlAckSeqType == "MU-BAR")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_TF_MU_BAR));
    }
    else if (dlAckSeqType == "AGGR-MU-BAR")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));
    }
    else if (dlAckSeqType != "NO-OFDMA")
    {
        NS_ABORT_MSG("Invalid DL ack sequence type (must be NO-OFDMA, ACK-SU-FORMAT, MU-BAR or "
                     "AGGR-MU-BAR)");
    }

    // main part of Simulation
    for (int randomSeed = 1; randomSeed < 10; randomSeed++)
    {
        std::cout << "Simulation randomSeed: " << randomSeed << " Start!" << std::endl;
        for (int muEDCAmultiplier = 2; muEDCAmultiplier <= 4;
             muEDCAmultiplier = muEDCAmultiplier * 2)
        {
            std::cout << "muEDCAmultiplier: " << muEDCAmultiplier << "   ";
            if (!udp)
            {
                Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
            }

            NodeContainer wifiStaNodes;
            wifiStaNodes.Create(nStations);

            NodeContainer wifiApNode;
            wifiApNode.Create(1);

            NetDeviceContainer apDevice;
            NetDeviceContainer staDevices;

            txsEnabled = (isEhtNetwork && txsEnabled)
                             ? txsEnabled
                             : false; // TXS supported is enabled in EHT network
            TxsWifiMacHelper mac(txsEnabled);
            WifiHelper wifi;
            std::string channelStr("{" + std::to_string(channelIndex) + ", " +
                                   std::to_string(channelWidth) + ", ");
            StringValue ctrlRate;
            auto nonHtRefRateMbps = HePhy::GetNonHtReferenceRate(mcs) / 1e6;

            std::ostringstream ossDataMode;
            ossDataMode << "HeMcs" << mcs;

            // Standard setting
            std::string netStandard;
            if (isEhtNetwork == true)
            {
                wifi.SetStandard(WIFI_STANDARD_80211be);
                wifi.ConfigEhtOptions(
                    "EmlsrActivated",
                    BooleanValue(false),
                    "TidToLinkMappingNegSupport",
                    EnumValue(
                        WifiTidToLinkMappingNegSupport::WIFI_TID_TO_LINK_MAPPING_NOT_SUPPORTED));

                netStandard = "ns3-80211be";
            }
            else
            {
                wifi.SetStandard(WIFI_STANDARD_80211ax);
                netStandard = "ns3-80211ax";
            }
            Ssid ssid = Ssid(netStandard);

            // Frequency Band (5 GHz: Second one)
            if (frequency == 6)
            {
                ctrlRate = StringValue(ossDataMode.str());
                channelStr += "BAND_6GHZ, " + std::to_string(primaryChannelIndex) + "}";
                Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss",
                                   DoubleValue(48));
            }
            else if (frequency == 5)
            {
                std::ostringstream ossControlMode;
                ossControlMode << "OfdmRate" << nonHtRefRateMbps << "Mbps";
                ctrlRate = StringValue(ossControlMode.str());
                channelStr += "BAND_5GHZ, " + std::to_string(primaryChannelIndex) + "}";
            }
            else if (frequency == 2.4)
            {
                std::ostringstream ossControlMode;
                ossControlMode << "ErpOfdmRate" << nonHtRefRateMbps << "Mbps";
                ctrlRate = StringValue(ossControlMode.str());
                channelStr += "BAND_2_4GHZ, " + std::to_string(primaryChannelIndex) + "}";
                Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss",
                                   DoubleValue(40));
            }
            else
            {
                std::cout << "Wrong frequency value!" << std::endl;
                return 0;
            }

            wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                         "DataMode",
                                         StringValue(ossDataMode.str()),
                                         "ControlMode",
                                         ctrlRate);

            // Set guard interval and MPDU buffer size
            wifi.ConfigHeOptions("GuardInterval",
                                 TimeValue(NanoSeconds(gi)),
                                 "MpduBufferSize",
                                 UintegerValue(useExtendedBlockAck ? 256 : 64),
                                 "BssColor",
                                 UintegerValue(1));

            // Set MU EDCA parameters
            wifi.ConfigHeOptions("MuViAifsn", // It must be either zero (EDCA disabled) or a value
                                              // from 2 to 15.
                                 UintegerValue(2 * muEDCAmultiplier),
                                 "MuVoAifsn",
                                 UintegerValue(2 * muEDCAmultiplier),
                                 "MuViCwMin", // It must be a power of 2 minus 1 in the range from 0
                                              // to 32767.
                                 UintegerValue(16 * muEDCAmultiplier - 1),
                                 "MuVoCwMin",
                                 UintegerValue(16 * muEDCAmultiplier - 1),
                                 "MuViCwMax",
                                 UintegerValue(1024 * muEDCAmultiplier - 1),
                                 "MuVoCwMax",
                                 UintegerValue(1024 * muEDCAmultiplier - 1),
                                 "BkMuEdcaTimer", // It must be a multiple of 8192 us and must be in
                                                  // the range from 8.192 ms to 2088.96 ms.
                                 TimeValue(MicroSeconds(8192)),
                                 "BeMuEdcaTimer",
                                 TimeValue(MicroSeconds(8192)),
                                 "ViMuEdcaTimer",
                                 TimeValue(MicroSeconds(8192)),
                                 "VoMuEdcaTimer",
                                 TimeValue(MicroSeconds(8192)));

            // Spectrum Channel Model

            Ptr<MultiModelSpectrumChannel> spectrumChannel =
                CreateObject<MultiModelSpectrumChannel>();

            Ptr<LogDistancePropagationLossModel> lossModel =
                CreateObject<LogDistancePropagationLossModel>();
            spectrumChannel->AddPropagationLossModel(lossModel);

            SpectrumWifiPhyHelper phy;
            phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
            phy.SetChannel(spectrumChannel);

            SpectrumNetworkInstallation(mac,
                                        phy,
                                        ssid,
                                        wifi,
                                        channelStr,
                                        wifiStaNodes,
                                        wifiApNode,
                                        staDevices,
                                        apDevice,
                                        dlAckSeqType,
                                        enableUlOfdma,
                                        enableBsrp,
                                        accessReqInterval);

            RngSeedManager::SetSeed(randomSeed);
            RngSeedManager::SetRun(1);
            int64_t streamNumber = 150;
            streamNumber += wifi.AssignStreams(apDevice, streamNumber);
            streamNumber += wifi.AssignStreams(staDevices, streamNumber);

            // Mobility
            InstallMobility(distanceBtw, distance, wifiApNode, wifiStaNodes, 0);

            /* Internet stack*/
            // AP -> All STA, STA -> AP
            InternetStackHelper stack;
            stack.Install(wifiApNode);
            stack.Install(wifiStaNodes);

            Ipv4AddressHelper address;
            address.SetBase("192.168.1.0", "255.255.255.0");
            Ipv4InterfaceContainer staNodeInterfaces;
            Ipv4InterfaceContainer apNodeInterface;

            staNodeInterfaces = address.Assign(staDevices);
            apNodeInterface = address.Assign(apDevice);

            /* Setting applications */
            ApplicationContainer serverApp;
            NodeContainer tempServerNodes;
            tempServerNodes.Add(wifiStaNodes, wifiApNode);
            auto serverNodes = std::ref(tempServerNodes);

            // AP send data to all STA, however, All STA just send data to AP only
            Ipv4InterfaceContainer serverInterfaces;
            NodeContainer clientNodes;
            for (std::size_t i = 0; i < nStations; i++)
            {
                serverInterfaces.Add(staNodeInterfaces.Get(i));
                serverInterfaces.Add(apNodeInterface.Get(0));
                clientNodes.Add(wifiApNode.Get(0));
                clientNodes.Add(wifiStaNodes.Get(i));
            }

            // Add packet access category (QoS)
            std::map<accessCategoryIndex, int> packetAcMap;
            packetAcMap[BE] = 0x70;
            packetAcMap[BK] = 0x28;
            packetAcMap[VI] = 0xb8;
            packetAcMap[VO] = 0xc0;

            std::string staApplicationDataRate = std::to_string(applicationDataRate) + "Mbps";
            std::string apApplicationDataRate = "";
            bool applicationDataRateFairness = true;
            int staNumberTransmission = nStations;
            if (applicationDataRateFairness)
                apApplicationDataRate =
                    std::to_string(applicationDataRate / staNumberTransmission) + "Mbps";
            else
                apApplicationDataRate = std::to_string(applicationDataRate) + "Mbps";

            if (udp)
            {
                // UDP flow
                uint16_t port = 9;
                UdpServerHelper server(port);
                serverApp = server.Install(serverNodes.get());
                serverApp.Start(Seconds(0.0));
                serverApp.Stop(Seconds(simulationTime + 1));

                for (std::size_t i = 0; i < nStations; i++)
                {
                    if (packetQosSupport)
                    {
                        // AP client
                        InetSocketAddress apDest(serverInterfaces.GetAddress(2 * i), port);
                        apDest.SetTos(packetAcMap[packetAc]);

                        OnOffHelper apClient("ns3::UdpSocketFactory", apDest);
                        apClient.SetAttribute(
                            "OnTime",
                            StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                        apClient.SetAttribute(
                            "OffTime",
                            StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                        apClient.SetAttribute("DataRate", StringValue(apApplicationDataRate));
                        apClient.SetAttribute("PacketSize", UintegerValue(payloadSize));

                        ApplicationContainer apClientApp = apClient.Install(clientNodes.Get(2 * i));
                        apClientApp.Start(Seconds(1.0));
                        apClientApp.Stop(Seconds(simulationTime + 1));

                        // STA client
                        InetSocketAddress staDest(serverInterfaces.GetAddress(2 * i + 1), port);
                        staDest.SetTos(packetAcMap[packetAc]);

                        OnOffHelper staClient("ns3::UdpSocketFactory", staDest);
                        staClient.SetAttribute(
                            "OnTime",
                            StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                        staClient.SetAttribute(
                            "OffTime",
                            StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                        staClient.SetAttribute("DataRate", StringValue(staApplicationDataRate));
                        staClient.SetAttribute("PacketSize", UintegerValue(payloadSize));

                        ApplicationContainer staClientApp =
                            staClient.Install(clientNodes.Get(2 * i + 1));
                        staClientApp.Start(Seconds(1.0));
                        staClientApp.Stop(Seconds(simulationTime + 1));
                    }
                }
            }

            // Data collection framework
            std::string probeTypePacket = "ns3::PacketProbe";
            std::string probeTypeIpv4Packet = "ns3::Ipv4PacketProbe";
            std::string probeTypeTime = "ns3::TimeProbe";

            std::string tracePathTx;
            std::string tracePathRx;
            std::string tracePathLatency;
            tracePathTx = "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEndQoS";
            tracePathRx = "/NodeList/*/$ns3::Ipv4L3Protocol/Rx";
            tracePathLatency = "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Phy/AvgDelayTime";

            // Use FileHelper to write out the packet byte count over time
            FileHelper fileHelperTx;
            FileHelper fileHelperRx;
            FileHelper fileHelperLatency;

            std::string headlineTx = "Scenario MU-UL: Tx QoS packet bytes";
            std::string headlineRx = "Scenario MU-UL: Rx packet bytes";
            std::string headlineLatency = "Scenario MU-UL: Avg latency time";
            fileHelperTx.SetHeading(headlineTx);
            fileHelperRx.SetHeading(headlineRx);
            fileHelperLatency.SetHeading(headlineLatency);

            // Configure the file to be written, and the formatting of output data.
            std::string filePathTx = "TXS-Data/MU-UL/randomSeed-" + std::to_string(randomSeed) +
                                     "-muEDCAmultiplier-" + std::to_string(muEDCAmultiplier) +
                                     "-macQueueSize-" + macQueueSize + "-apApplicationDataRate-" +
                                     apApplicationDataRate + "-staApplicationDataRate-" +
                                     staApplicationDataRate + "-tx-packet";
            std::string filePathRx = "TXS-Data/MU-UL/randomSeed-" + std::to_string(randomSeed) +
                                     "-muEDCAmultiplier-" + std::to_string(muEDCAmultiplier) +
                                     "-macQueueSize-" + macQueueSize + "-apApplicationDataRate-" +
                                     apApplicationDataRate + "-staApplicationDataRate-" +
                                     staApplicationDataRate + "-rx-packet";
            std::string filePathLatency =
                "TXS-Data/MU-UL/randomSeed-" + std::to_string(randomSeed) + "-muEDCAmultiplier-" +
                std::to_string(muEDCAmultiplier) + "-macQueueSize-" + macQueueSize +
                "-apApplicationDataRate-" + apApplicationDataRate + "-staApplicationDataRate-" +
                staApplicationDataRate + "-latency-time";

            fileHelperTx.ConfigureFile(filePathTx, FileAggregator::FORMATTED);
            fileHelperRx.ConfigureFile(filePathRx, FileAggregator::FORMATTED);
            fileHelperLatency.ConfigureFile(filePathLatency, FileAggregator::FORMATTED);

            fileHelperTx.Set2dFormat("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");
            fileHelperRx.Set2dFormat("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");
            fileHelperLatency.Set2dFormat("Time (Seconds) = %.3e\tAvg Latency Time = %.3e");

            fileHelperTx.WriteProbe(probeTypePacket, tracePathTx, "OutputBytes");
            fileHelperRx.WriteProbe(probeTypeIpv4Packet, tracePathRx, "OutputBytes");
            fileHelperLatency.WriteProbe(probeTypeTime, tracePathLatency, "Output");

            // Simulation run
            Simulator::Schedule(Seconds(0), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);
            Simulator::Stop(Seconds(simulationTime + 1));

            Simulator::Run();
            Simulator::Destroy();
            std::cout << "Done" << std::endl;
        }
    }
    return 0;
}
