#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Mywireless");

int main (int argc, char *argv[])
{
	bool verbose = true;
	uint32_t nWifi = 3;
	bool tracing = true;

	CommandLine cmd;
	cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);
	cmd.AddValue("verbose", "Tell the echo application to log if true", verbose);
	cmd.AddValue("tracing", "Enable pcap tracing", tracing);

	cmd.Parse(argc, argv);

	if (nWifi > 250)
	{
		std::cout << "Too many wifi nodes, no more than 250 each." << std::endl;
		return 1;
	}

	if (verbose)
	{
		LogComponentEnable ("Mywireless", LOG_LEVEL_INFO);
	}

	NodeContainer wifiApNode;
	wifiApNode.Create(1);
	NodeContainer wifiStaNodes;
	wifiStaNodes.Create(nWifi);

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	phy.SetChannel(channel.Create());

	WifiHelper wifi;
	wifi.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue("VhtMcs9"), "ControlMode", StringValue("VhtMcs0"));

	Ssid ssid = Ssid ("ns3-wifi-80211n");

	WifiMacHelper mac;

	mac.SetType("ns3::StaWifiMac",
			"Ssid", SsidValue (ssid),
			"ActiveProbing", BooleanValue (false));

	NetDeviceContainer staDevices;
	staDevices = wifi.Install(phy, mac, wifiStaNodes);

	mac.SetType("ns3::ApWifiMac",
			"Ssid", SsidValue (ssid));

	NetDeviceContainer apDevices;
	apDevices = wifi.Install(phy, mac, wifiApNode);

	MobilityHelper mobility;

	mobility.SetPositionAllocator("ns3::GridPositionAllocator",
			"MinX", DoubleValue (0.0),
			"MinY", DoubleValue (0.0),
			"DeltaX", DoubleValue (5.0),
			"DeltaY", DoubleValue (10.0),
			"GridWidth", UintegerValue (3),
			"LayoutType", StringValue ("RowFirst"));

	mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
			"Bounds", RectangleValue (Rectangle (-100,100,-100,100)));
	mobility.Install (wifiStaNodes);

	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);

	InternetStackHelper stack;
	stack.Install(wifiStaNodes);
	stack.Install(wifiApNode);

	Ipv4AddressHelper address;

	address.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer apInterfaces;
	apInterfaces = address.Assign(apDevices);
	Ipv4InterfaceContainer staInterfaces;
	staInterfaces = address.Assign(staDevices);

	NS_LOG_INFO ("Create Application");

	OnOffHelper clientHelper ("ns3::UdpSocketFactory", Address());
	clientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
	clientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	clientHelper.SetAttribute("PacketSize", UintegerValue (25));
	clientHelper.SetAttribute("DataRate", DataRateValue (DataRate ("2500kb/s")));

	ApplicationContainer clientApps = clientHelper.Install(wifiStaNodes);
	clientApps.Start(Seconds (1.0));
	clientApps.Stop(Seconds (10.0));

	PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", Address(InetSocketAddress (Ipv4Address::GetAny(), 9)));
	ApplicationContainer clientSink = sinkHelper.Install(wifiApNode);

	clientSink.Start(Seconds (1.0));
	clientSink.Stop(Seconds (10.0));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	Simulator::Stop(Seconds (10.0));

	if (tracing == true)
	{
		phy.EnablePcap ("mywireless", apDevices.Get(0));
		phy.EnablePcap("mywireless", staDevices.Get(1));
	}

	Simulator::Run();
	Simulator::Destroy();
	return 0;
}
