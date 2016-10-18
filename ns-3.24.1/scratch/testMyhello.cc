//std::cout << Simulator::Now ().GetSeconds () << " HelloTimerExpire::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";

#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/wave-module.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/nqos-wifi-mac-helper.h"
#include "ns3/myhello-module.h"
#include "ns3/wifi-80211p-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("test-DECA-protocol");
/*
Ptr<Socket>
SetupPacketReceive (Ipv4Address addr, Ptr<Node> node, Ptr<cobra::RoutingProtocol> test)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  sink->SetRecvCallback (MakeCallback (&cobra::RoutingProtocol::RecvDataPacket, test));
  InetSocketAddress local = InetSocketAddress (addr, 9);
  sink->Bind (local);
  return sink;
}
*/
int main (int argc, char *argv[])
{
	// 3600 5 nodes test.tcl
	// 5400 4 nodes test2.tcl
	// 7200 3 nodes test3.tcl
	
  //std::string NS2traceFile = "/home/admiral/Desktop/backupmyroot/bkk_pathumwan_[9-12_FRI]_[08To10].tcl";  /// traceMobilityNS2 format file's name
  std::string NS2traceFile = "/home/toonnp/Desktop/urban_grid_4X4/g3_v2_4.tcl";  /// traceMobilityNS2 format file's name
  int nWifis = 47;           /// Number of Nodes
  double TotalTime = 120;   /// Simulation Time
  double txp = 32;          /// Transmit Power  34 = 500m
  //uint32_t packetSize = 256; /// bytes
  bool verbose = false;
  std::string phyMode ("OfdmRate6MbpsBW10MHz");   ///PhyMode 802.11p
  //std::string phyMode ("DsssRate11Mbps");
  
  CommandLine cmd;
  cmd.AddValue ("NS2traceFile", "Ns2 movement trace file.", NS2traceFile);
  //cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);
  cmd.AddValue ("nWifis", "Number of nodes.", nWifis);
  cmd.AddValue ("TotalTime", "Total simulation time.", TotalTime);
  //cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.Parse (argc, argv);
  
  // disable fragmentation for frames below 2200 bytes
  //Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  //Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  //Set Non-unicastMode rate to unicast mode
  //Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));
  
  /// Create Nodes
  NodeContainer srcNode;
  srcNode.Create (1);
  
  NodeContainer adhocNodes;
  adhocNodes.Create (nWifis);
  
  /// setting up wifi channel using helpers
 YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
 wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
 //wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel");
 //wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel",
  //                                "Distance1" ,DoubleValue (250), "m1", DoubleValue (0.0), "m2", DoubleValue (0.0));
  //wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel","Distance1",DoubleValue(600),
                                 //"Distance2",DoubleValue(1000),"m0",DoubleValue(5),"m1",DoubleValue(0.1),"m2",DoubleValue(5.5));
  //wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(450));
 // wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel");
  //wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel");
  
  /// setting up wifi phy using helpers
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  //wifiPhy.SetStandard (WIFI_PHY_STANDARD_80211p_CCH); 
  wifiPhy.SetChannel (channel);
  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  wifiPhy.Set("TxGain",DoubleValue(0)); 
  wifiPhy.Set("RxGain",DoubleValue(0));  
  //wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-101.0));
  //wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
  
  /// Add a non-QoS upper mac, and disable rate control
  //NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  NqosWaveMacHelper wifiMac = NqosWaveMacHelper::Default();
  //wifiMac.SetType ("ns3::AdhocWifiMac");
  
  /// setting up wifi standard using helpers
 // WifiHelper wifi;
 // wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
 //                               "DataMode",StringValue (phyMode),
 //                               "ControlMode",StringValue (phyMode));
 Wifi80211pHelper wifi = Wifi80211pHelper::Default ();
 wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                     "DataMode",StringValue (phyMode),
                                     "ControlMode",StringValue (phyMode));
 
 if (verbose)
	wifi.EnableLogComponents ();  
 
  /// Create Wifi devices       
  NetDeviceContainer srcDevices = wifi.Install (wifiPhy, wifiMac, srcNode);
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);
  
  //wifiPhy.EnablePcap ("wave-simple-80211p", adhocDevices);
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  
  positionAlloc->Add (Vector (16.35, 18.34, 0.0));
  //positionAlloc->Add (Vector (550.0, 0.0, 0.0));
  //positionAlloc->Add (Vector (-500.0, 0.0, 0.0));
  
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (srcNode); 
  
  Ns2MobilityHelper ns2 = Ns2MobilityHelper (NS2traceFile);
  ns2.Install ();
  
  
  
  // import movements from ns2 tracefile
  /*
  /// Create Ns2MobilityHelper with the specified trace log file as parameter
  Ns2MobilityHelper ns2 = Ns2MobilityHelper (NS2traceFile);
  /// configure movements for each node, while reading trace file
  ns2.Install ();
  */
  /// Create Internet stack and Routing protocol
  MyhelloHelper agent;
  InternetStackHelper internet;
  internet.SetRoutingHelper (agent);
  internet.Install (srcNode);
  internet.Install (adhocNodes);
  
  /// create interfaces and assign address
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer srcInterfaces;
  srcInterfaces = addressAdhoc.Assign (srcDevices);
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);
  /*
  for (int i = 0; i < Ori; i++)
    {
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i), adhocNodes.Get (i),
                                             (adhocNodes.Get (i))->GetObject<cobra::RoutingProtocol> ());
    }
  */
  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();
  Simulator::Destroy ();
  
  return 0;
}

