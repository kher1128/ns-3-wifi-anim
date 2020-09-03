/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Vikas Pushkar (Adapted from third.cc)
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/basic-energy-source.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/wifi-radio-energy-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Wireless Environment for Graduation");

int 
main (int argc, char *argv[])
{
  uint32_t nWifi = 5;                                                  //20 >> 10 End point 개수
  double distance = 5;
  CommandLine cmd (__FILE__);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);          
  cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);

  cmd.Parse (argc,argv);
  NodeContainer allNodes;                                               // 모든 ap, end point, csma 노드를 저장하는 컨테이너                        
  NodeContainer wifiStaNodes[2];                                           //Sta = standard
  wifiStaNodes[0].Create (nWifi);

  wifiStaNodes[1].Create (2);
  allNodes.Add (wifiStaNodes[1]);
  
  allNodes.Add (wifiStaNodes[0]);
  NodeContainer wifiApNode[2] ;
  wifiApNode[0].Create (4);                                                //AP 개수 
  allNodes.Add (wifiApNode[0]);                                            

  wifiApNode[1].Create(1);  
  allNodes.Add(wifiApNode[1]);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();    //Wifi channel helper
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();                //phy helper phy는 계층, mac도 계층임
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");                // 딱히 들어가는거 의미 없는듯

  WifiMacHelper mac;                                                    // ap에 ssid, activeprobing 설정
  Ssid ssid = Ssid ("ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));                  

  NetDeviceContainer staDevices[2];
  
  
  staDevices[0] = wifi.Install (phy, mac, wifiStaNodes[0]);                                          // standard device wifi에 인스톨 한다
  staDevices[1] = wifi.Install (phy, mac, wifiStaNodes[1]);


  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));                                                           // mac을 wifi으로 세팅하고 이전에 설정한 ssid값 삽입

  NetDeviceContainer apDevices[2];                                                                   // apDevice에 연결
  apDevices[0] = wifi.Install (phy, mac, wifiApNode[0]);                        //
  apDevices[1] = wifi.Install (phy, mac, wifiApNode[1]);

  // Mobility

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (10.0),                                              // 단말 노드 위치 지정
                                 "MinY", DoubleValue (0.0),                                              
                                 "DeltaX", DoubleValue (10.0),                                             // 노드가 반복되서 설치 되는데 이거는 각 노드들의 거리
                                 "DeltaY", DoubleValue (2.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",                                            //단말 노드들의 움직임 랜덤하게 움직이는 범위 지정
                             "Bounds", RectangleValue (Rectangle (0, 100, -20, 50)));
  mobility.Install (wifiStaNodes[0]);

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (10.0),                                              // 단말 노드 위치 지정
                                 "MinY", DoubleValue (40.0),                                              
                                 "DeltaX", DoubleValue (5.0),                                             // 노드가 반복되서 설치 되는데 이거는 각 노드들의 거리
                                 "DeltaY", DoubleValue (2.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",                                            //단말 노드들의 움직임 랜덤하게 움직이는 범위 지정
                             "Bounds", RectangleValue (Rectangle (0, 100, -20, 50)));
  mobility.Install (wifiStaNodes[1]);


  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");                                       //ap는 고정 되있음
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (12.0),                                             
                                 "MinY", DoubleValue (10.0),                                              //ap 노드들의 위치 설정
                                 "DeltaX", DoubleValue (20.0),
                                 "DeltaY", DoubleValue (2.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.Install (wifiApNode[0]);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");                                       //ap는 고정 되있음
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (8.0),                                             
                                 "MinY", DoubleValue (25.0),                                              //ap 노드들의 위치 설정
                                 "DeltaX", DoubleValue (10.0),
                                 "DeltaY", DoubleValue (2.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.Install (wifiApNode[1]);  
                                                                       
  Ptr<BasicEnergySource> energySource = CreateObject<BasicEnergySource>();
  Ptr<WifiRadioEnergyModel> energyModel = CreateObject<WifiRadioEnergyModel>();

  energySource->SetInitialEnergy (600);
  energyModel->SetEnergySource (energySource);
  energySource->AppendDeviceEnergyModel (energyModel);

  // aggregate energy source to node
  wifiApNode[0].Get (0)->AggregateObject (energySource);


  Ptr<BasicEnergySource> energySource2 = CreateObject<BasicEnergySource>();
  Ptr<WifiRadioEnergyModel> energyModel2 = CreateObject<WifiRadioEnergyModel>();

  energySource2->SetInitialEnergy (600);
  energyModel2->SetEnergySource (energySource2);
  energySource2->AppendDeviceEnergyModel (energyModel2);

  wifiApNode[1].Get (0)->AggregateObject (energySource2);
  // Install internet stack

  InternetStackHelper stack;
  stack.Install (allNodes);

  // Install Ipv4 addresses

  Ipv4AddressHelper address;                                                                      //노드들 역할 알아보고 노드 주소 설정 다시 해야할듯
  address.SetBase("192.168.1.0", "255.255.255.0");

  Ipv4InterfaceContainer staInterfaces[2];
  staInterfaces[0] = address.Assign (staDevices[0]);
  staInterfaces[1] = address.Assign (staDevices[1]);
  //address.SetBase ("164.125.180.0", "255.255.255.0");

  Ipv4InterfaceContainer apInterface[2];
  apInterface[0] = address.Assign (apDevices[0]);
  apInterface[1] = address.Assign (apDevices[1]);

  
  std::cout<<"before server setting\n";
  UdpServerHelper myServer (9);
	ApplicationContainer serverApp;
  serverApp = myServer.Install (wifiStaNodes[0].Get (0));
	serverApp.Start (Seconds (0.0));
	serverApp.Stop (Seconds (15.0));

  // serverApp[1] = myServer.Install (wifiStaNodes[1].Get (0));
	// serverApp[1].Start (Seconds (0.0));
	// serverApp[1].Stop (Seconds (15.0));

  std::cout<<"before client setting\n";
	UdpClientHelper myClient (staInterfaces[0].GetAddress (0), 9);
	myClient.SetAttribute ("MaxPackets", UintegerValue (987654321));	// Maximum number of packets = 2^32
	myClient.SetAttribute ("Interval", TimeValue (Time ("0.00002"))); //packets/s
	myClient.SetAttribute ("PacketSize", UintegerValue (256));

  std::cout<<"before clientapp setting\n";

	ApplicationContainer clientApps;
  clientApps = myClient.Install (wifiApNode[0].Get (0));
  // clientApps = myClient2.Install (wifiApNode[1].Get (0));
  clientApps.Start (Seconds (2.0));                                                                       //클라이언트도 ON
  clientApps.Start (Seconds (2.0));  
                                                                       //클라이언트도 ON
  std::cout<<"before routingtable setting\n";

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop (Seconds (15.0));
  std::cout<<"before anim setting\n";

  AnimationInterface anim ("wireless-animation.xml"); // Mandatory                                         // 파일 생성 뒤에 숫자를 넣어서 패킷 포함 개수 설정
  for (uint32_t i = 0; i < wifiStaNodes[0].GetN (); ++i)                                                     
    {
      anim.UpdateNodeDescription (wifiStaNodes[0].Get (i), "End"); // Optional                               //wifiStaNodes[0] 에 "End" 텍스트 할당, 색깔 지정
      anim.UpdateNodeColor (wifiStaNodes[0].Get (i), 255, 0, 0); // Optional 색깔
      
    }
  for (uint32_t i = 0; i < wifiStaNodes[1].GetN (); ++i)                                                     
    {
      anim.UpdateNodeDescription (wifiStaNodes[1].Get (i), "End"); // Optional                               //wifiStaNodes[0] 에 "End" 텍스트 할당, 색깔 지정
      anim.UpdateNodeColor (wifiStaNodes[1].Get (i), 255, 0, 0); // Optional 색깔
    }
  anim.UpdateNodeDescription (wifiApNode[0].Get (0), "SAP-33.233"); // Optional
  anim.UpdateNodeDescription (wifiApNode[0].Get (1), "16K-313-2-2"); // Optional
  anim.UpdateNodeDescription (wifiApNode[0].Get (2), "SAP-33.234"); // Optional
  anim.UpdateNodeDescription (wifiApNode[0].Get (3), "16K-313-2-3"); // Optional

  for (uint32_t i = 0; i < wifiApNode[0].GetN (); ++i)
    {
      anim.UpdateNodeColor (wifiApNode[0].Get (i), 0, 255, 0); // Optional
    }

  
  anim.UpdateNodeDescription (wifiApNode[1].Get (0), "16K-313-2-1"); // Optional
  anim.UpdateNodeColor (wifiApNode[1].Get (0), 0, 255, 0); // Optional

  // anim.EnablePacketMetadata (); // Optional                                                                               //패킷 데이터 저장
  // anim.EnableIpv4RouteTracking ("routingtable-wireless.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
  // anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
  // anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
