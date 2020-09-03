#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleMpduAggregation");

int main (int argc, char *argv[])
{
	uint32_t payloadSize = 1472; //bytes
	uint64_t simulationTime = 10; //seconds

	CommandLine cmd;
	cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
	cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
	cmd.Parse (argc, argv);

	// 1. Create Nodes
	NodeContainer wifiStaNode;
	wifiStaNode.Create (1);
	NodeContainer wifiApNode;
	wifiApNode.Create (1);

	// 2. Create PHY layer (wireless channel)
	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.SetChannel (channel.Create ());

	// 3. Create MAC layer
	WifiMacHelper mac;
	Ssid ssid = Ssid ("Section6");
	mac.SetType ("ns3::StaWifiMac",
			"Ssid", SsidValue (ssid),
			"ActiveProbing", BooleanValue (false));

	// 4. Create WLAN setting
	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("HtMcs7"), "ControlMode", StringValue ("HtMcs0"));

	// 5. Create NetDevices
	NetDeviceContainer staDevice;
	staDevice = wifi.Install (phy, mac, wifiStaNode);

	mac.SetType ("ns3::ApWifiMac",
			"Ssid", SsidValue (ssid),
			"BeaconInterval", TimeValue (MicroSeconds (102400)),
			"BeaconGeneration", BooleanValue (true));

	NetDeviceContainer apDevice;
	apDevice = wifi.Install (phy, mac, wifiApNode);

	// 6. Create Network layer
	/* Internet stack*/
	InternetStackHelper stack;
	stack.Install (wifiApNode);
	stack.Install (wifiStaNode);

	Ipv4AddressHelper address;

	address.SetBase ("192.168.1.0", "255.255.255.0");
	Ipv4InterfaceContainer StaInterface;
	StaInterface = address.Assign (staDevice);
	Ipv4InterfaceContainer ApInterface;
	ApInterface = address.Assign (apDevice);

	// 7. Locate nodes
	/* Setting mobility model */
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

	positionAlloc->Add (Vector (0.0, 0.0, 0.0));
	positionAlloc->Add (Vector (1.0, 0.0, 0.0));
	// mobility.SetPositionAllocator (positionAlloc, "GridWidth", UintegerValue (5));
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (3.0),                                             //10 >> 5  단말 노드 위치 지정
                                 "MinY", DoubleValue (5.0),                                              //ap 노드들의 위치를 어떻게 바꿀 수 있을까
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (2.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));

	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

	mobility.Install (wifiApNode);
	mobility.Install (wifiStaNode);

	// 8. Create Transport layer (UDP)
	/* Setting applications */
	UdpServerHelper myServer (9);
	ApplicationContainer serverApp = myServer.Install (wifiStaNode.Get (0));
	serverApp.Start (Seconds (0.0));
	serverApp.Stop (Seconds (simulationTime + 1));

	UdpClientHelper myClient (StaInterface.GetAddress (0), 9);
	myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295));	// Maximum number of packets = 2^32
	myClient.SetAttribute ("Interval", TimeValue (Time ("0.00002"))); //packets/s
	myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));

	ApplicationContainer clientApp = myClient.Install (wifiApNode.Get (0));
	clientApp.Start (Seconds (1.0));
	clientApp.Stop (Seconds (simulationTime + 1));

    AnimationInterface anim ("wlan-animation.xml"); // Mandatory
  for (uint32_t i = 0; i < wifiStaNode.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiStaNode.Get (i), "STA"); // Optional
      anim.UpdateNodeColor (wifiStaNode.Get (i), 255, 0, 0); // Optional
    }
  for (uint32_t i = 0; i < wifiApNode.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiApNode.Get (i), "PNU_WiFi"); // Optional
      anim.UpdateNodeColor (wifiApNode.Get (i), 0, 255, 0); // Optional
    }
// 9. Simulation Run and Calc. throughput
	Simulator::Stop (Seconds (simulationTime + 1));
	Simulator::Run ();
	Simulator::Destroy ();

	uint32_t totalPacketsRecv = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
	double throughput = totalPacketsRecv * payloadSize * 8 / (simulationTime * 1000000.0);
	std::cout << "Throughput: " << throughput << " Mbps" << '\n';

	return 0;
}

