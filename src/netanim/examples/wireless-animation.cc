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
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("WirelessAnimationExample");
 
int main (int argc, char *argv[])
{
  int nSta = 70;
  int nAp = 4;
  // int nAp = 3;

  double apX[4] = {12.5, 37.5, 12.5, 37.5};
  double apY[4] = {12.5, 12.5, 37.5, 37.5};
  // double apX[nAp] = {12.5, 37.5, 12.5};
  // double apY[nAp] = {12.5, 12.5, 37.5};

  string ssids[4] = {"ns-3-ssid1",
                       "ns-3-ssid2",
                       "ns-3-ssid3"
                       "ns-3-ssid4"};
  // string ssids[4] = {"ns-3-ssid1",
  //                      "ns-3-ssid2",
  //                      "ns-3-ssid3"}; 

  string ips[4] = {"10.1.2.0",
                     "10.1.3.0",
                     "10.1.4.0",
                     "10.1.5.0"};
  // string ips[nAp] = {"10.1.2.0",
  //                    "10.1.3.0",
  //                    "10.1.4.0"};

  // LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  // LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer allNodes;
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nSta);
  allNodes.Add (wifiStaNodes);
  NodeContainer wifiApNodes;
  wifiApNodes.Create (nAp);
  allNodes.Add(wifiApNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (wifiApNodes);
  csmaNodes.Create (1);
  allNodes.Add (csmaNodes.Get (nAp));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  // Install internet stack
  InternetStackHelper stack;
  stack.Install (allNodes);

  // Install Ipv4 addresses
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  // Mobility
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                "MinX", DoubleValue (1),
                                "MinY", DoubleValue (1),
                                "DeltaX", DoubleValue (8),
                                "DeltaY", DoubleValue (8),
                                "GridWidth", UintegerValue (6),
                                "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue (Rectangle (0,50,0,50)));
  mobility.Install (wifiStaNodes);  
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNodes);

  for (int i = 0; i < nAp ; i++)
  {
    AnimationInterface::SetConstantPosition (wifiApNodes.Get(i), apX[i], apY[i]);
  }  
  AnimationInterface::SetConstantPosition (csmaNodes.Get(nAp), 25, 25); 

  list<int> minDistAp;
  for (int i = 0 ; i < nSta ; i++)
  {
    int minJ = 0;
    double minDistance = 0;
    for (int j = 0 ; j < nAp ; j++ )
    {
      Ptr<MobilityModel> model1 = wifiApNodes.Get(j)->GetObject<MobilityModel>();
      Ptr<MobilityModel> model2 = wifiStaNodes.Get(i)->GetObject<MobilityModel>();
      if ( j ==0 )
      {
        minDistance = model1->GetDistanceFrom(model2);
        minJ = j;
      } 
      else
      {
        double distance = model1->GetDistanceFrom (model2);
        if (distance < minDistance) 
        {
          minDistance = distance;
          minJ = j;
        }
      }
    }
    minDistAp.push_back(minJ);
  }                   

  list<NetDeviceContainer> staDevicesList;
  list<NetDeviceContainer> apDevicesList;
  for (int i = 0 ; i < nAp ; i++)
  {
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());

    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");    

    WifiMacHelper mac;
    Ssid ssid = Ssid (ssids[i].c_str());
    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevices;
    NodeContainer wifiAssocStaNodes;
    for (list<int>::iterator iter = minDistAp.begin() ; iter != minDistAp.end() ; iter++)
    {
      if ( *iter == i) 
      {
        int index = distance(minDistAp.begin(), iter);
        wifiAssocStaNodes.Add(wifiStaNodes.Get(index));
      }
    }

    staDevices = wifi.Install (phy, mac, wifiAssocStaNodes);
    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install (phy, mac, wifiApNodes.Get(i));

    staDevicesList.push_back(staDevices);
    apDevicesList.push_back(apDevices);
  }

  for ( int i = 0 ; i < nAp ; i++ ) 
  {
    list<NetDeviceContainer>::iterator iter;
    address.SetBase (ips[i].c_str(), "255.255.255.0");

    iter = staDevicesList.begin();
    advance(iter, i);
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign (*iter);
    
    iter = apDevicesList.begin();
    advance(iter, i);    
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign (*iter);
  }

  // Install applications

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nAp));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (15.0));
  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nAp), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  ApplicationContainer clientApps = echoClient.Install (wifiStaNodes);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (15.0));

  // Install FlowMonitor on csma nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop (Seconds (15.0));

  AnimationInterface anim ("wireless-animation.xml"); // Mandatory
  for (uint32_t i = 0; i < wifiStaNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiStaNodes.Get (i), "STA"); // Optional
      anim.UpdateNodeColor (wifiStaNodes.Get (i), 255, 0, 0); // Optional
    }
  for (uint32_t i = 0; i < wifiApNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiApNodes.Get (i), "AP"); // Optional
      anim.UpdateNodeColor (wifiApNodes.Get (i), 0, 255, 0); // Optional
    }
  anim.UpdateNodeDescription (csmaNodes.Get (nAp), "SERVER"); // Optional
  anim.UpdateNodeColor (csmaNodes.Get (nAp), 0, 0, 255); // Optional 

  anim.EnablePacketMetadata (); // Optional
  anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
  anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional

  csma.EnablePcapAll("wireless-animation", false);

  Simulator::Run ();

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  {
  //     if (i->first > 2)
  //     {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        cout << "  Tx Packets: " << i->second.txPackets << "\n";
        cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
        cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
  //     }
  }

  Simulator::Destroy ();
  return 0;
}
