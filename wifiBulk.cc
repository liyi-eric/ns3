/*
 * wifiBulk.cc
 *
 *  Created on: 2018-4-17
 *      Author: eric
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("myWirelessBulk");

int main (int argc, char *argv[])
{
	bool verbose = true;
	double simulationTime = 5;//seconds
	double frequency = 5.0;//whethre 2.4 or 5.0 GHz
	uint32_t nWifi = 10;
	bool shortGuardInterval = false;
	bool channelBonding = false;

	CommandLine cmd;
	cmd.AddValue("verbose", "Tell the echo application to log if true", verbose);
	cmd.AddValue("simulationTime", "Simulation time (in seconds)", simulationTime);
	cmd.AddValue("frequency", "Whether woriking in the 2.4 or 5.0 GHz band(other values gets rejected", frequency);
	cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
	cmd.AddValue("shortGuardInterval","Enable/disable short guard interval", shortGuardInterval);
	cmd.AddValue("channelBonding", "Enable/disable channel bonding(channel width = 20MHz if false, channel width = 40MHz if true", channelBonding);

	cmd.Parse(argc, argv);

	if (nWifi > 250)
	{
		std::cout << "Too many wifi STA nodes, no more than 249 each." << std::endl;
		return 1;
	}

	if (verbose)
	{
		LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
		LogComponentEnable ("ConstantRateWifiManager", LOG_LEVEL_INFO);
	}

	NodeContainer wifiApNode;
	wifiApNode.Create(1);
	NodeContainer wifiStaNodes;
	wifiStaNodes.Create(nWifi);



	YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	phy.SetChannel(channel.Create());
	phy.Set("ShortGuardEnabled", BooleanValue(shortGuardInterval));

	WifiMacHelper mac;
	WifiHelper wifi;

	if (frequency == 5.0)
	{
		wifi.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);
	}
	else if (frequency == 2.4)
	{
		wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
	}
	else
	{
		std::cout << "Wrong frequency Values!" << std::endl;
		return 0;
	}

	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("HtMcs7"),
			"ControlMode", StringValue("HtMcs7"));

	Ssid ssid = Ssid ("ns3-80211n");

	mac.SetType("ns3::StaWifiMac",
			"Ssid", SsidValue(ssid),
			"ActiveProbing", BooleanValue(true));

	NetDeviceContainer staDevices;
	staDevices = wifi.Install(phy, mac, wifiStaNodes);

	mac.SetType("ns3::ApWifiMac",
			"Ssid", SsidValue(ssid));

	NetDeviceContainer apDevices;
	apDevices = wifi.Install(phy, mac, wifiApNode);

	if (channelBonding)
	{
		Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));
	}

	MobilityHelper mobility;

	mobility.SetPositionAllocator("ns3::GridPositionAllocator",
				"MinX", DoubleValue (0.0),
				"MinY", DoubleValue (0.0),
				"DeltaX", DoubleValue (5.0),
				"DeltaY", DoubleValue (10.0),
				"GridWidth", UintegerValue (3),
				"LayoutType", StringValue ("RowFirst"));

	mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
				"Bounds", RectangleValue (Rectangle (-50,50,-50,50)));
	mobility.Install (wifiStaNodes);

	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);

	InternetStackHelper stack;
	stack.Install(wifiApNode);
	stack.Install(wifiStaNodes);

	Ipv4AddressHelper address;

	address.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer apInterface;
	Ipv4InterfaceContainer staInterfaces;
	apInterface = address.Assign(apDevices);
	staInterfaces= address.Assign(staDevices);

	uint16_t port = 50000;
	Address localAddress (InetSocketAddress(Ipv4Address::GetAny(), port));
	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", localAddress);
	ApplicationContainer sinker = sinkHelper.Install(wifiApNode);
	sinker.Start(Seconds (1.0));
	sinker.Stop(Seconds (simulationTime + 1));

	AddressValue remoteAddress (InetSocketAddress(apInterface.GetAddress(0), port));
	BulkSendHelper sources ("ns3::TcpSocketFactory", Address());
	sources.SetAttribute("Remote", remoteAddress);
	sources.SetAttribute("SendSize", UintegerValue(25));
	sources.SetAttribute("MaxBytes", UintegerValue (0));
	ApplicationContainer sourcesApps = sources.Install(wifiStaNodes);
	sourcesApps.Start(Seconds (1.0));
	sourcesApps.Stop(Seconds (simulationTime + 1));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	Simulator::Stop(Seconds (simulationTime + 1));
	Simulator::Run();
	Simulator::Destroy();

	return 0;
}


