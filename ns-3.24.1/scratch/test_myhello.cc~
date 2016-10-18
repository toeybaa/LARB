#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/myhello-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("test-MYHELLO-protocol");

int main (int argc, char *argv[])
{
  //std::string NS2traceFile = "/home/vanet-cu/Documents/urban_grid_4X4/g3_v2_1.tcl";  /// traceMobilityNS2 format file's name
  int nWifis = 2;           /// Number of Nodes
  double TotalTime = 100;    /// Simulation Time
  double txp = 18;          /// Transmit Power
  std::string rate ("2048bps");
  std::string phyMode ("DsssRate11Mbps");
  
  // Enable logging from the ns2 helper
  //LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_DEBUG);
  
  CommandLine cmd;
  //cmd.AddValue ("traceFile", "Ns2 movement trace file", NS2traceFile);
  cmd.AddValue ("nWifis", "number of nodes", nWifis);
  cmd.AddValue ("TotalTime", "Simulation Time", TotalTime);
  cmd.Parse (argc, argv);
  
  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));
  //Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  //Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));
  
  /// Create Nodes
  NodeContainer adhocNodes;
  adhocNodes.Create (nWifis);
  
  /// setting up wifi channel using helpers
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
  
  /// setting up wifi phy using helpers
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));
  
  /// Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");
  
  /// setting up wifi standard using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  /// Create Wifi devices       
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);

  // import movements from ns2 tracefile
  /// Create Ns2MobilityHelper with the specified trace log file as parameter
  //Ns2MobilityHelper ns2 = Ns2MobilityHelper (NS2traceFile);
  /// configure movements for each node, while reading trace file
  //ns2.Install ();
  
  /// configure movements
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (100.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (adhocNodes);
  
  /// Create Internet stack and Routing protocol
  MyhelloHelper myhello;
  InternetStackHelper internet;
  internet.SetRoutingHelper (myhello);
  internet.Install (adhocNodes);
  
  /// create interfaces and assign address
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.0.0.0", "255.255.0.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);
  /*
  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
  */
  
  /*
  AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (0), 9));
  onoff1.SetAttribute ("Remote", remoteAddress);

  //Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
  ApplicationContainer temp = onoff1.Install (adhocNodes.Get (1));
  //temp.Start (Seconds (var->GetValue (10.0,11.0)));
  temp.Start (Seconds (10.0));
  temp.Stop (Seconds (TotalTime));
  */
  
  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();
  Simulator::Destroy ();
  
  return 0;
}

