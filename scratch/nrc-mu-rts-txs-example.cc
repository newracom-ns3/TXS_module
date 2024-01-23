/*
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
 * @cm.lee author 뒤에 수정할 것.
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
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-acknowledgment.h"

// @cm.lee
#include "ns3/stats-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("nrc-mu-rts-txs-example_cm");

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
    // mobility.
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

void
ShowPosition(NodeContainer& nodes)
{
    for (auto node = nodes.Begin(); node != nodes.End(); ++node)
    {
        uint32_t nodeId = (*node)->GetId();
        Ptr<MobilityModel> mobModel = (*node)->GetObject<MobilityModel>();
        Vector3D pos = mobModel->GetPosition();
        Vector3D speed = mobModel->GetVelocity();
        std::cout << "At " << Simulator::Now().GetSeconds() << " node " << nodeId << ": Position("
                  << pos.x << ", " << pos.y << ", " << pos.z << ");   Speed(" << speed.x << ", "
                  << speed.y << ", " << speed.z << ")" << std::endl;

        // Simulator::Schedule(Seconds(0), &ShowPosition, node);
    }
}

// @cm.lee dictionary처럼 BE = 0x70 바로 되는 방식은 없나?
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
    // Customized part
    LogComponentEnable("NrcFrameExchangeManager", LOG_LEVEL_INFO);
    LogComponentEnable("NrcCtrlTriggerHeader", LOG_LEVEL_INFO);
    // LogComponentEnable("HeFrameExchangeManager", LOG_LEVEL_INFO);

    // QoS
    accessCategoryIndex packetAc{VI}; // BE, BK, VI, VO
    bool packetQosSupport{true};

    // Phy
    std::string phyModel{"Spectrum"};
    bool fixedPhyBand{true};
    if (fixedPhyBand == true)
        Config::SetDefault("ns3::WifiPhy::FixedPhyBand", BooleanValue(true));
    int channelIndex = 42;
    int primaryChannelIndex = 3; // value range = [0, 3]
    double frequency{5};         // whether 2.4, 5 or 6 GHz
    bool multiChannelWidthFlag = false;
    uint8_t maxChannelWidthIn5G_6G{80}; // Bandwidth = 20, 40, 80, 160 [MHz]
    uint8_t maxChannelWidth = maxChannelWidthIn5G_6G;
    int gi{800}; // Guard interval = 3200, 1600, 800 [ms]
    int mcs{5};  // -1 indicates an unset value
    bool multiMcsModeFlag = false;
    int maxMcs = 5;

    // Mac
    bool useRts{true};
    bool useExtendedBlockAck{false};
    Time accessReqInterval{0}; // AP MAC

    // Mobility
    double distanceBtw{10.0};
    bool showPositionFlag{false};
    double distance{1.0}; // meters

    // Network setting
    bool isEhtNetwork{true};
    bool udp{true};
    bool downlink{true};
    bool enableUlOfdma{false}; // TXS-example disable
    bool enableBsrp{false};
    std::size_t nStations{4};
    std::string dlAckSeqType{"NO-OFDMA"}; // MU-BAR은 현재 리시버 주소 겹치는 문제가 있음.
    uint32_t payloadSize = 700;

    // Simulation and Tracing
    double simulationTime{4}; // seconds
    bool tracing{true};
    double minExpectedThroughput{0};
    double maxExpectedThroughput{0};

    // input (뒤에 지워도 될 듯)
    CommandLine cmd(__FILE__);
    cmd.AddValue("frequency",
                 "Whether working in the 2.4, 5 or 6 GHz band (other values gets rejected)",
                 frequency);
    cmd.AddValue("distance",
                 "Distance in meters between the station and the access point",
                 distance);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
    cmd.AddValue("downlink",
                 "Generate downlink flows if set to 1, uplink flows otherwise",
                 downlink);
    cmd.AddValue("useRts", "Enable/disable RTS/CTS", useRts);
    cmd.AddValue("useExtendedBlockAck", "Enable/disable use of extended BACK", useExtendedBlockAck);
    cmd.AddValue("nStations", "Number of non-AP HE stations", nStations);
    cmd.AddValue("dlAckType",
                 "Ack sequence type for DL OFDMA (NO-OFDMA, ACK-SU-FORMAT, MU-BAR, AGGR-MU-BAR)",
                 dlAckSeqType);
    cmd.AddValue("enableUlOfdma",
                 "Enable UL OFDMA (useful if DL OFDMA is enabled and TCP is used)",
                 enableUlOfdma);
    cmd.AddValue("enableBsrp",
                 "Enable BSRP (useful if DL and UL OFDMA are enabled and TCP is used)",
                 enableBsrp);
    cmd.AddValue(
        "muSchedAccessReqInterval",
        "Duration of the interval between two requests for channel access made by the MU scheduler",
        accessReqInterval);
    cmd.AddValue("mcs", "if set, limit testing to a specific MCS (0-11)", mcs);
    cmd.AddValue("payloadSize", "The application payload size in bytes", payloadSize);
    cmd.AddValue("phyModel",
                 "PHY model to use when OFDMA is disabled (Yans or Spectrum). If OFDMA is enabled "
                 "then Spectrum is automatically selected",
                 phyModel);
    cmd.AddValue("minExpectedThroughput",
                 "if set, simulation fails if the lowest throughput is below this value",
                 minExpectedThroughput);
    cmd.AddValue("maxExpectedThroughput",
                 "if set, simulation fails if the highest throughput is above this value",
                 maxExpectedThroughput);
    cmd.Parse(argc, argv);

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

    if (phyModel != "Yans" && phyModel != "Spectrum")
    {
        NS_ABORT_MSG("Invalid PHY model (must be Yans or Spectrum)");
    }
    if (dlAckSeqType != "NO-OFDMA")
    {
        // SpectrumWifiPhy is required for OFDMA
        phyModel = "Spectrum";
    }

    // double prevThroughput[12] = {0};

    std::cout << "MCS value"
              << "\t\t"
              << "Channel width"
              << "\t\t"
              << "GI"
              << "\t\t\t"
              << "Throughput" << '\n';

    int minMcs = (multiMcsModeFlag == true) ? 0 : maxMcs;
    uint8_t minChannelWidth = (multiChannelWidthFlag == true) ? 20 : maxChannelWidth;

    for (int mcs = minMcs; mcs <= maxMcs; mcs++)
    {
        // uint8_t index = 0;
        // double previous = 0;
        for (int channelWidth = minChannelWidth; channelWidth <= maxChannelWidth;
             channelWidth++) // MHz
        {
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

            WifiMacHelper mac;
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

            wifi.ConfigHeOptions(
                "MuViAifsn", // It must be either zero (EDCA disabled) or a value from 2 to 15.
                UintegerValue(10),
                "MuVoAifsn",
                UintegerValue(5),
                "MuViCwMin", // It must be a power of 2 minus 1 in the range from 0 to 32767.
                UintegerValue(31),
                "MuVoCwMin",
                UintegerValue(31),
                "MuViCwMax",
                UintegerValue(1023),
                "MuVoCwMax",
                UintegerValue(1023),
                "BkMuEdcaTimer", // It must be a multiple of 8192 us and must be in
                                 // the range from 8.192 ms to 2088.96 ms.
                TimeValue(MicroSeconds(8192)),
                "BeMuEdcaTimer",
                TimeValue(MicroSeconds(8192)),
                "ViMuEdcaTimer",
                TimeValue(MicroSeconds(8192)),
                "VoMuEdcaTimer",
                TimeValue(MicroSeconds(8192)));

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

            Ssid ssid = Ssid(netStandard);

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

            // @cm.lee 랜덤 시드 바꾸는 부분 --> 시뮬레이션 결과 비슷하게 나오면 바꿀 것.
            RngSeedManager::SetSeed(1);
            RngSeedManager::SetRun(1);
            int64_t streamNumber = 150;
            streamNumber += wifi.AssignStreams(apDevice, streamNumber);
            streamNumber += wifi.AssignStreams(staDevices, streamNumber);

            // Mobility
            InstallMobility(distanceBtw, distance, wifiApNode, wifiStaNodes, 0);

            if (showPositionFlag == true)
            {
                ShowPosition(wifiApNode);
                ShowPosition(wifiStaNodes);
            }

            /* Internet stack*/
            // AP -> All STA (full buffer), All STA -> AP (full buffer)
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
            // @cm.lee server listen data (AP + all STA need to be added)
            ApplicationContainer serverApp;
            NodeContainer tempServerNodes;
            tempServerNodes.Add(wifiStaNodes, wifiApNode);
            auto serverNodes = std::ref(tempServerNodes);

            // @cm.lee client sent data (AP + all STA need to be added)
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

            std::cout << "clientNodes 개수: " << clientNodes.GetN() << "\n";
            std::cout << "serverNodes 개수: " << tempServerNodes.GetN() << "\n";

            // @sm.lee add packet access category (QoS)
            std::map<accessCategoryIndex, int> packetAcMap;
            packetAcMap[BE] = 0x70;
            packetAcMap[BK] = 0x28;
            packetAcMap[VI] = 0xb8;
            packetAcMap[VO] = 0xc0;

            if (udp)
            {
                // UDP flow
                uint16_t port = 9;
                UdpServerHelper server(port);
                serverApp = server.Install(serverNodes.get());
                serverApp.Start(Seconds(0.0));
                serverApp.Stop(Seconds(simulationTime + 1));

                // @cm.lee
                for (std::size_t i = 0; i < nStations; i++)
                {
                    // @sm.lee add packet access category
                    // @cm.lee AP and STA send same traffic profile, if we need to change it please
                    // check below
                    if (packetQosSupport)
                    {
                        // @cm.lee AP client (AP send data setting)
                        InetSocketAddress apDest(serverInterfaces.GetAddress(2 * i), port);
                        apDest.SetTos(packetAcMap[packetAc]);

                        OnOffHelper apClient("ns3::UdpSocketFactory", apDest);
                        apClient.SetAttribute(
                            "OnTime",
                            StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                        apClient.SetAttribute(
                            "OffTime",
                            StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                        apClient.SetAttribute("DataRate", StringValue("100000kb/s"));
                        apClient.SetAttribute("PacketSize", UintegerValue(payloadSize));

                        ApplicationContainer apClientApp = apClient.Install(clientNodes.Get(2 * i));
                        std::cout << "AP-clientApp: " << apClientApp.Get(0)
                                  << " IP address: " << serverInterfaces.GetAddress(2 * i)
                                  << " Port: " << port << " node: " << clientNodes.Get(2 * i)
                                  << " applications number: "
                                  << clientNodes.Get(2 * i)->GetNApplications() << std::endl;
                        apClientApp.Start(Seconds(1.0));
                        apClientApp.Stop(Seconds(simulationTime + 1));

                        // @cm.lee STA client (STA send data setting)
                        InetSocketAddress staDest(serverInterfaces.GetAddress(2 * i + 1), port);
                        staDest.SetTos(packetAcMap[packetAc]);

                        OnOffHelper staClient("ns3::UdpSocketFactory", staDest);
                        staClient.SetAttribute(
                            "OnTime",
                            StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                        staClient.SetAttribute(
                            "OffTime",
                            StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                        staClient.SetAttribute(
                            "DataRate",
                            StringValue("100000kb/s")); // Maybe we need to make this slow
                        staClient.SetAttribute("PacketSize", UintegerValue(payloadSize));

                        ApplicationContainer staClientApp =
                            staClient.Install(clientNodes.Get(2 * i + 1));
                        std::cout << "STA-clientApp: " << staClientApp.Get(0)
                                  << " IP address: " << serverInterfaces.GetAddress(2 * i + 1)
                                  << " Port: " << port << " node: " << clientNodes.Get(2 * i + 1)
                                  << " applications number: "
                                  << clientNodes.Get(2 * i + 1)->GetNApplications() << std::endl;
                        staClientApp.Start(Seconds(1.0));
                        staClientApp.Stop(Seconds(simulationTime + 1));
                    }
                    else
                    {
                        // @cm.lee 뒤에 필요하면 손 볼 것.
                        UdpClientHelper client(serverInterfaces.GetAddress(i), port);
                        client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
                        client.SetAttribute("Interval", TimeValue(Time("0.00001"))); // packets/s
                        ApplicationContainer clientApp = client.Install(clientNodes.Get(i));
                        std::cout << "clientApp: " << clientApp.Get(0)
                                  << " IP address: " << serverInterfaces.GetAddress(i)
                                  << " Port: " << port << " node: " << clientNodes.Get(i)
                                  << " applications number: "
                                  << clientNodes.Get(i)->GetNApplications() << std::endl;
                        clientApp.Start(Seconds(1.0));
                        clientApp.Stop(Seconds(simulationTime + 1));
                    }
                }
                std::cout << "AP UDP Application number (server + client): "
                          << clientNodes.Get(0)->GetNApplications() << std::endl;
                std::cout << "STA 1 (Shared STA) UDP Application number (server + client): "
                          << clientNodes.Get(1)->GetNApplications() << std::endl;
            }

            /*
            Data collection framework
            */

            std::string probeType;
            std::string tracePath_tx;
            std::string tracePath_rx;

            probeType = "ns3::Ipv4PacketProbe";

            tracePath_tx = "/NodeList/*/$ns3::Ipv4L3Protocol/Tx";
            tracePath_rx = "/NodeList/*/$ns3::Ipv4L3Protocol/Rx";

            // Use FileHelper to write out the packet byte count over time
            FileHelper fileHelperTx;
            FileHelper fileHelperRx;

            std::string headlineTx = "Scenario TXS: Tx packet bytes";
            std::string headlineRx = "Scenario TXS: Rx packet bytes";
            fileHelperTx.SetHeading(headlineTx);
            fileHelperRx.SetHeading(headlineRx);

            // If we try multiple simulation, please check this
            int simTrial = 0;

            // Configure the file to be written, and the formatting of output data.
            std::string filePathTx = "newracom-data/TXS/tx-packet-" + std::to_string(simTrial);
            std::string filePathRx = "newracom-data/TXS/rx-packet-" + std::to_string(simTrial);
            std::cout << "Tx packet file path: " << filePathTx << std::endl;
            std::cout << "Rx packet file path: " << filePathRx << std::endl;
            fileHelperTx.ConfigureFile(filePathTx, FileAggregator::FORMATTED);
            fileHelperRx.ConfigureFile(filePathRx, FileAggregator::FORMATTED);

            // Set the labels for this formatted output file.
            fileHelperTx.Set2dFormat("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");
            fileHelperRx.Set2dFormat("Time (Seconds) = %.3e\tPacket Byte Count = %.0f");

            // Specify the probe type, trace source path (in configuration namespace), and
            // probe output trace source ("OutputBytes") to write.
            fileHelperTx.WriteProbe(probeType, tracePath_tx, "OutputBytes");
            fileHelperRx.WriteProbe(probeType, tracePath_rx, "OutputBytes");

            // Simulation run
            Simulator::Schedule(Seconds(0), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);
            Simulator::Stop(Seconds(simulationTime + 1));

            if (tracing)
            {
                phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
                phy.EnablePcap("wifi-he-network_homogeneous_two_aps", apDevice.Get(0));
                phy.EnablePcap("wifi-he-network_homogeneous_two_aps", staDevices.Get(0), true);
            }

            Simulator::Run();

            // When multiple stations are used, there are chances that association requests
            // collide and hence the throughput may be lower than expected. Therefore, we relax
            // the check that the throughput cannot decrease by introducing a scaling factor (or
            // tolerance)
            uint64_t rxBytes = 0;

            if (udp)
            {
                for (uint32_t i = 0; i < serverApp.GetN(); i++)
                {
                    rxBytes +=
                        payloadSize * DynamicCast<UdpServer>(serverApp.Get(i))->GetReceived();
                }
            }

            double throughput = (rxBytes * 8) / (simulationTime * 1000000.0); // Mbit/s

            Simulator::Destroy();

            std::cout << mcs << "\t\t\t" << channelWidth << " MHz\t\t\t" << gi << " ns\t\t\t"
                      << throughput << " Mbit/s" << std::endl;
        }
    }
    return 0;
}
