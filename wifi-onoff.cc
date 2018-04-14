#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MywirelessOnoff");

int main (int argc, char *argv[])
{
	bool verbose = true;
	uint32_t nWifi = 1;
	bool tracing = false;

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
		LogComponentEnable ("MywirelessOnoff", LOG_LEVEL_INFO);
		LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
	}

	NodeContainer wifiApNode;
	wifiApNode.Create(1);
	NodeContainer wifiStaNodes;
	wifiStaNodes.Create(nWifi);

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	phy.SetChannel(channel.Create());
	phy.Set ("ShortGuardEnabled", BooleanValue(true));

	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211ac);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
			StringValue("VhtMcs9"), "ControlMode", StringValue("VhtMcs0"));

	Ssid ssid = Ssid ("ns3-wifi-80211ac");

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
			"Bounds", RectangleValue (Rectangle (-50,50,-50,50)));
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

	uint16_t port = 50000;
	Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny(), port));
	PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);

	ApplicationContainer clientSink = sinkHelper.Install(wifiApNode.Get(0));
	clientSink.Start(Seconds (1.0));
	clientSink.Stop(Seconds (10.0));

	OnOffHelper clientHelper ("ns3::UdpSocketFactory",Address());
	AddressValue remoteAddress (InetSocketAddress(apInterfaces.GetAddress(0), port));
	clientHelper.SetAttribute("Remote", remoteAddress);
	clientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
	clientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	clientHelper.SetAttribute("PacketSize", UintegerValue (25));
	clientHelper.SetAttribute("DataRate", StringValue ("20000b/s"));

	ApplicationContainer clientApps = clientHelper.Install(wifiStaNodes.Get(0));

	clientApps.Start(Seconds (1.0));
	clientApps.Stop(Seconds (10.0));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	Simulator::Stop(Seconds (10.0));

	if (tracing == true)
	{
		phy.EnablePcap ("mywirelessonoff", apDevices.Get(0));
		phy.EnablePcap("mywirelessonoff", staDevices.Get(0));
	}

	Simulator::Run();
	Simulator::Destroy();
	return 0;
}
