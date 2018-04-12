#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Text1ScriptExample");

int
main (int argc, char *argv[])
{
	bool verbose = true;
	uint32_t nWifi = 10;

	CommandLine cmd;
	cmd.AddValue ("nWifi", "Number of wifi devices", nWifi);
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

	cmd.Parse(argc,argv);

	if (nWifi > 250)
	{
		std::cout << "Too many wifi nodes, no more than 250 each." << std::endl;
		return 1;
	}

	if (verbose)
	{
		LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
		LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
	}

	NodeContainer wifiApNode;
	wifiApNode.Create(1);
	NodeContainer wifiStaNodes;
	wifiStaNodes.Create (nWifi);

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	phy.SetChannel(channel.Create());

	WifiHelper wifi;
	wifi.SetRemoteStationManager("ns3::AarfWifiManager");

	WifiMacHelper mac;
	Ssid ssid = Ssid ("ns-3-ssid");
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

	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
			"Bounds", RectangleValue (Rectangle (-100, 100, -100, 100)));

	mobility.Install (wifiStaNodes);

	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);

	InternetStackHelper stack;
	stack.Install (wifiApNode);
	stack.Install (wifiStaNodes);

	Ipv4AddressHelper address;

	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer wifiInterfaces;
	wifiInterfaces = address.Assign (apDevices);
	address.Assign (staDevices);

	UdpEchoServerHelper echoServer (9);

	ApplicationContainer serverApps = echoServer.Install (wifiApNode.Get(0));
	serverApps.Start(Seconds (1.0));
	serverApps.Stop(Seconds (10.0));

	UdpEchoClientHelper echoClient (wifiInterfaces.GetAddress(0), 9);
	echoClient.SetAttribute("MaxPackets", UintegerValue (10));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (25));

	ApplicationContainer clientApps = echoClient.Install (wifiStaNodes.Get (nWifi));
	clientApps.Start(Seconds (2.0));
	clientApps.Stop(Seconds (10.0));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	Simulator::Stop(Seconds (10.0));

	Simulator::Run ();
	Simulator::Destroy();
	return 0;
}
