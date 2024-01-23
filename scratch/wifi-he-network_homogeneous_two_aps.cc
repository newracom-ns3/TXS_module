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
 */

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
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
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <functional>

// @cm.lee
#include "ns3/stats-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("he-wifi-network");

void
YansNetworkInstallation(WifiMacHelper& mac,
                        YansWifiPhyHelper& phy,
                        Ssid& ssid,
                        WifiHelper& wifi,
                        std::string& channelStr,
                        NodeContainer& wifiStaNodes,
                        NodeContainer& wifiApNode,
                        NetDeviceContainer& staDevices,
                        NetDeviceContainer& apDevice)
{
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    phy.Set("ChannelSettings", StringValue(channelStr));
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "EnableBeaconJitter",
                BooleanValue(false),
                "Ssid",
                SsidValue(ssid));
    apDevice = wifi.Install(phy, mac, wifiApNode);
};

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

int
main(int argc, char* argv[])
{
    // Customized part
    LogComponentEnable("HeFrameExchangeManager", LOG_LEVEL_INFO);

    bool fixedPhyBand{false};
    if (fixedPhyBand == true)
        Config::SetDefault("ns3::WifiPhy::FixedPhyBand", BooleanValue(true));
    int channelIndex = 42;
    int obssChannelIndex = 42;
    int primaryChannelIndex = 0;         // value range = [0, 3]
    int obssPrimaryChannelIndex = 0;     // value range = [0, 3]
    double packetInterval = 0.00001;     // original value = 0.00001
    double packetIntervalObss = 0.00001; // original value = 0.00001
    bool obssOperation{false};
    double distanceBtw{10.0};
    double frequency{5}; // whether 2.4, 5 or 6 GHz
    bool tracing{false};
    bool showPositionFlag{false};
    bool multiMcsModeFlag{false};
    bool multiChannelWidthFlag{false};
    int gi{1600};                       // Guard interval = 3200, 1600, 800 [ms]
    uint8_t maxChannelWidthIn5G_6G{80}; // Bandwidth = 20, 40, 80, 160 [MHz]
    uint8_t maxChannelWidthIn2_4G{40};  // Bandwidth = 20, 40 [MHz]
    uint8_t maxChannelWidth = (frequency == 2.4) ? maxChannelWidthIn2_4G : maxChannelWidthIn5G_6G;
    int maxMcs = 11;

    // Original part
    bool udp{true};
    bool downlink{true};
    bool useRts{true};
    bool useExtendedBlockAck{false};
    double simulationTime{4}; // seconds
    double distance{1.0};     // meters
    std::size_t nStations{4};
    std::string dlAckSeqType{"MU-BAR"};
    bool enableUlOfdma{false};
    bool enableBsrp{false};
    int mcs{-1}; // -1 indicates an unset value
    uint32_t payloadSize =
        700; // must fit in the max TX duration when transmitting at MCS 0 over an RU of 26 tones
    std::string phyModel{"Spectrum"};
    double minExpectedThroughput{0};
    double maxExpectedThroughput{0};
    Time accessReqInterval{0};

    CommandLine cmd(__FILE__);
    cmd.AddValue("obssOperation",
                 "if there is an obss network, obssOperation = true",
                 obssOperation);
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

    double prevThroughput[12] = {0};

    std::cout << "MCS value"
              << "\t\t"
              << "Channel width"
              << "\t\t"
              << "GI"
              << "\t\t\t"
              << "Throughput" << '\n';

    int minMcs = (multiMcsModeFlag == true) ? 0 : maxMcs;
    uint8_t minChannelWidth = (multiChannelWidthFlag == true) ? 20 : maxChannelWidth;

    int simTrial = 0;
    for (int mcs = minMcs; mcs <= maxMcs; mcs++)
    {
        uint8_t index = 0;
        double previous = 0;

        for (int channelWidth = minChannelWidth; channelWidth <= maxChannelWidth;) // MHz
        {
            if (!udp)
            {
                Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
            }

            NodeContainer wifiStaNodes, wifiStaNodesObss;
            wifiStaNodes.Create(nStations);
            wifiStaNodesObss.Create(nStations);
            NodeContainer wifiApNode, wifiApNodeObss;
            wifiApNode.Create(1);
            wifiApNodeObss.Create(1);

            NetDeviceContainer apDevice, apDeviceObss;
            NetDeviceContainer staDevices, staDevicesObss;
            WifiMacHelper mac;
            WifiHelper wifi;
            std::string channelStr("{" + std::to_string(channelIndex) + ", " +
                                   std::to_string(channelWidth) + ", ");
            std::string channelStrObss("{" + std::to_string(obssChannelIndex) + ", " +
                                       std::to_string(channelWidth) + ", ");
            StringValue ctrlRate;
            auto nonHtRefRateMbps = HePhy::GetNonHtReferenceRate(mcs) / 1e6;

            std::ostringstream ossDataMode;
            ossDataMode << "HeMcs" << mcs;

            if (frequency == 6)
            {
                wifi.SetStandard(WIFI_STANDARD_80211ax);
                ctrlRate = StringValue(ossDataMode.str());
                channelStr += "BAND_6GHZ, " + std::to_string(primaryChannelIndex) + "}";
                channelStrObss += "BAND_6GHZ, " + std::to_string(obssPrimaryChannelIndex) + "}";
                Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss",
                                   DoubleValue(48));
            }
            else if (frequency == 5)
            {
                wifi.SetStandard(WIFI_STANDARD_80211ax);
                std::ostringstream ossControlMode;
                ossControlMode << "OfdmRate" << nonHtRefRateMbps << "Mbps";
                ctrlRate = StringValue(ossControlMode.str());
                channelStr += "BAND_5GHZ, " + std::to_string(primaryChannelIndex) + "}";
                channelStrObss += "BAND_5GHZ, " + std::to_string(obssPrimaryChannelIndex) + "}";
            }
            else if (frequency == 2.4)
            {
                wifi.SetStandard(WIFI_STANDARD_80211ax);
                std::ostringstream ossControlMode;
                ossControlMode << "ErpOfdmRate" << nonHtRefRateMbps << "Mbps";
                ctrlRate = StringValue(ossControlMode.str());
                channelStr += "BAND_2_4GHZ, " + std::to_string(primaryChannelIndex) + "}";
                channelStrObss += "BAND_2_4GHZ, " + std::to_string(obssPrimaryChannelIndex) + "}";
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
                                 UintegerValue(useExtendedBlockAck ? 256 : 64));

            Ssid ssid = Ssid("ns3-80211ax_1");
            Ssid ssidObss = Ssid("ns3-80211ax_2");

            /*
             * SingleModelSpectrumChannel cannot be used with 802.11ax because two
             * spectrum models are required: one with 78.125 kHz bands for HE PPDUs
             * and one with 312.5 kHz bands for, e.g., non-HT PPDUs (for more details,
             * see issue #408 (CLOSED))
             */

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
            if (obssOperation == true)
            {
                SpectrumNetworkInstallation(mac,
                                            phy,
                                            ssidObss,
                                            wifi,
                                            channelStrObss,
                                            wifiStaNodesObss,
                                            wifiApNodeObss,
                                            staDevicesObss,
                                            apDeviceObss,
                                            dlAckSeqType,
                                            enableUlOfdma,
                                            enableBsrp,
                                            accessReqInterval);
            }

            RngSeedManager::SetSeed(1);
            RngSeedManager::SetRun(1);
            int64_t streamNumber = 150;
            streamNumber += wifi.AssignStreams(apDevice, streamNumber);
            streamNumber += wifi.AssignStreams(staDevices, streamNumber);
            if (obssOperation == true)
            {
                streamNumber += wifi.AssignStreams(apDeviceObss, streamNumber);
                streamNumber += wifi.AssignStreams(staDevicesObss, streamNumber);
            }

            uint32_t NAps = wifiApNode.GetN();
            if (obssOperation == true)
                NAps += wifiApNodeObss.GetN();

            InstallMobility(distanceBtw, distance, wifiApNode, wifiStaNodes, 0);

            if (obssOperation == true)
            {
                InstallMobility(distanceBtw, distance, wifiApNodeObss, wifiStaNodesObss, 1);
            }
            if (showPositionFlag == true)
            {
                ShowPosition(wifiApNode);
                ShowPosition(wifiStaNodes);
                ShowPosition(wifiApNodeObss);
                ShowPosition(wifiStaNodesObss);
            }

            /* Internet stack*/
            InternetStackHelper stack;
            stack.Install(wifiApNode);
            stack.Install(wifiStaNodes);

            if (obssOperation == true)
            {
                stack.Install(wifiApNodeObss);
                stack.Install(wifiStaNodesObss);
            }

            Ipv4AddressHelper address;
            address.SetBase("192.168.1.0", "255.255.255.0");
            Ipv4InterfaceContainer staNodeInterfaces, staNodeInterfacesObss;
            Ipv4InterfaceContainer apNodeInterface, apNodeInterfaceObss;

            staNodeInterfaces = address.Assign(staDevices);
            apNodeInterface = address.Assign(apDevice);

            if (obssOperation == true)
            {
                staNodeInterfacesObss = address.Assign(staDevicesObss);
                apNodeInterfaceObss = address.Assign(apDeviceObss);
            }

            /* Setting applications */
            ApplicationContainer serverApp, serverAppObss;
            auto serverNodes = downlink ? std::ref(wifiStaNodes) : std::ref(wifiApNode);
            auto serverNodesObss = downlink ? std::ref(wifiStaNodesObss) : std::ref(wifiApNodeObss);

            Ipv4InterfaceContainer serverInterfaces, serverInterfacesObss;
            NodeContainer clientNodes, clientNodesObss;

            for (std::size_t i = 0; i < nStations; i++)
            {
                serverInterfaces.Add(downlink ? staNodeInterfaces.Get(i) : apNodeInterface.Get(0));
                clientNodes.Add(downlink ? wifiApNode.Get(0) : wifiStaNodes.Get(i));
            }

            if (obssOperation == true)
            {
                for (std::size_t i = 0; i < nStations; i++)
                {
                    serverInterfacesObss.Add(downlink ? staNodeInterfacesObss.Get(i)
                                                      : apNodeInterfaceObss.Get(0));
                    clientNodesObss.Add(downlink ? wifiApNodeObss.Get(0) : wifiStaNodesObss.Get(i));
                }
            }

            if (udp)
            {
                // UDP flow
                uint16_t port = 9, portObss = 10;
                UdpServerHelper server(port);
                serverApp = server.Install(serverNodes.get());
                serverApp.Start(Seconds(0.0));
                serverApp.Stop(Seconds(simulationTime + 1));

                for (std::size_t i = 0; i < nStations; i++)
                {
                    UdpClientHelper client(serverInterfaces.GetAddress(i), port);
                    client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
                    client.SetAttribute(
                        "Interval",
                        TimeValue(Time(std::to_string(packetInterval)))); // packets/s
                    client.SetAttribute("PacketSize", UintegerValue(payloadSize));
                    ApplicationContainer clientApp = client.Install(clientNodes.Get(i));
                    // ApplicationContainer clientApp = client.Install(clientNodes.Get(0));
                    clientApp.Start(Seconds(1.0));
                    clientApp.Stop(Seconds(simulationTime + 1));
                }
                if (obssOperation == true)
                {
                    UdpServerHelper server(portObss);
                    serverAppObss = server.Install(serverNodesObss.get());
                    serverAppObss.Start(Seconds(1));
                    serverAppObss.Stop(Seconds(simulationTime + 1));

                    for (std::size_t i = 0; i < nStations; i++)
                    {
                        UdpClientHelper client(serverInterfacesObss.GetAddress(i), portObss);
                        client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
                        client.SetAttribute(
                            "Interval",
                            TimeValue(Time(std::to_string(packetIntervalObss)))); // packets/s
                        client.SetAttribute("PacketSize", UintegerValue(payloadSize));
                        ApplicationContainer clientApp = client.Install(clientNodesObss.Get(i));
                        // ApplicationContainer clientApp = client.Install(clientNodes.Get(0));
                        clientApp.Start(Seconds(1.0));
                        clientApp.Stop(Seconds(simulationTime + 1));
                    }
                }
            }

            /*
             Data collection framework
            */

            std::string probeType;
            std::string tracePath_mu_rts;
            probeType = "ns3::Uinteger32Probe";

            tracePath_mu_rts =
                "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/"
                "FrameExchangeManager/$ns3::HeFrameExchangeManager/NumberofSendMuRts";

            // Use FileHelper to write out the packet byte count over time
            FileHelper fileHelper;

            // Configure the file to be written, and the formatting of output data.
            std::string filePath = "newracom-data/data2/muRts-" + std::to_string(simTrial);
            simTrial++;
            std::cout << filePath << "\n";
            fileHelper.ConfigureFile(filePath, FileAggregator::FORMATTED);

            // Set the labels for this formatted output file.
            fileHelper.Set2dFormat(
                "Time (Seconds) = %.3e, The total number of sending MU-RTS = %.f");

            // Specify the probe type, trace source path (in configuration namespace), and
            // probe output trace source ("OutputBytes") to write.
            // fileHelper.WriteProbe(probeType, tracePath_rx, "OutputBytes");
            fileHelper.WriteProbe(probeType, tracePath_mu_rts, "Output");

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
            double tolerance = 0.10;
            uint64_t rxBytes = 0;
            if (udp)
            {
                for (uint32_t i = 0; i < serverApp.GetN(); i++)
                {
                    rxBytes +=
                        payloadSize * DynamicCast<UdpServer>(serverApp.Get(i))->GetReceived();
                }
                if (obssOperation == true)
                {
                    for (uint32_t i = 0; i < serverAppObss.GetN(); i++)
                    {
                        rxBytes += payloadSize *
                                   DynamicCast<UdpServer>(serverAppObss.Get(i))->GetReceived();
                    }
                }
            }

            double throughput = (rxBytes * 8) / (simulationTime * 1000000.0); // Mbit/s

            Simulator::Destroy();

            std::cout << mcs << "\t\t\t" << channelWidth << " MHz\t\t\t" << gi << " ns\t\t\t"
                      << throughput << " Mbit/s" << std::endl;

            // test first element
            if (mcs == 0 && channelWidth == 20 && gi == 3200)
            {
                if (throughput * (1 + tolerance) < minExpectedThroughput)
                {
                    NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                    exit(1);
                }
            }
            // test last element
            if (mcs == 11 && channelWidth == 160 && gi == 800)
            {
                if (maxExpectedThroughput > 0 &&
                    throughput > maxExpectedThroughput * (1 + tolerance))
                {
                    NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                    exit(1);
                }
            }
            // Skip comparisons with previous cases if more than one stations are present
            // because, e.g., random collisions in the establishment of Block Ack agreements
            // have an impact on throughput
            if (nStations == 1)
            {
                // test previous throughput is smaller (for the same mcs)
                if (throughput * (1 + tolerance) > previous)
                {
                    previous = throughput;
                }
                else if (throughput > 0)
                {
                    NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                    exit(1);
                }
                // test previous throughput is smaller (for the same channel width and GI)
                if (throughput * (1 + tolerance) > prevThroughput[index])
                {
                    prevThroughput[index] = throughput;
                }
                else if (throughput > 0)
                {
                    NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                    exit(1);
                }
            }
            index++;
            channelWidth *= 2;
        }
    }
    return 0;
}
