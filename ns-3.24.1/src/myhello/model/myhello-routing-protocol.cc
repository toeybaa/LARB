/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 */
#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_ipv4) { std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; } 

#include "myhello-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/udp-header.h"
#include "ns3/node-list.h"
#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/mobility-model.h"
#include <algorithm>
#include <limits>
#include <math.h>       /* ceil */

#define PKT_EXPIRE 5000.0
#define isBeacon true
#define isData true
#define beaconInv 1 // Second
#define dataInv 30 // Second
#define floodingHop false // Turn on flooding + Hop protocol
#define floodingGPS false // Turn on flooding + GPS protocol
#define decaGPS false
#define coverageRange 2000 
#define maxRange 500
#define isAdaptiveHopCounter true

NS_LOG_COMPONENT_DEFINE ("MyhelloRoutingProtocol");

namespace ns3
{
namespace myhello
{
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for MYHELLO control traffic
const uint32_t RoutingProtocol::MYHELLO_PORT = 667;

//-----------------------------------------------------------------------------
/// Tag used by MYHELLO implementation

class DeferredRouteOutputTag : public Tag
{

public:
  DeferredRouteOutputTag (int32_t o = -1) : Tag (), m_oif (o) {}

  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::myhello::DeferredRouteOutputTag").SetParent<Tag> ()
      .SetParent<Tag> ()
      .AddConstructor<DeferredRouteOutputTag> ()
    ;
    return tid;
  }

  TypeId  GetInstanceTypeId () const 
  {
    return GetTypeId ();
  }

  int32_t GetInterface() const
  {
    return m_oif;
  }

  void SetInterface(int32_t oif)
  {
    m_oif = oif;
  }

  uint32_t GetSerializedSize () const
  {
    return sizeof(int32_t);
  }

  void  Serialize (TagBuffer i) const
  {
    i.WriteU32 (m_oif);
  }

  void  Deserialize (TagBuffer i)
  {
    m_oif = i.ReadU32 ();
  }

  void  Print (std::ostream &os) const
  {
    os << "DeferredRouteOutputTag: output interface = " << m_oif;
  }

private:
  /// Positive if output device is fixed in RouteOutput
  int32_t m_oif;
};

NS_OBJECT_ENSURE_REGISTERED (DeferredRouteOutputTag);


//-----------------------------------------------------------------------------
RoutingProtocol::RoutingProtocol () :
  HelloInterval (Seconds (beaconInv)),
  DataInterval (Seconds (dataInv)),
  MaxQueueLen (64),
  MaxQueueTime (Seconds (30)),
  EnableHello (isBeacon),
  EnableData (isData),
  m_neighborTable (),
 // m_nb (HelloInterval),
  m_queue (MaxQueueLen, MaxQueueTime),
  m_htimer (Timer::CANCEL_ON_DESTROY),
  m_datatimer (Timer::CANCEL_ON_DESTROY),
  m_brtimer (Timer::CANCEL_ON_DESTROY),
  m_dtimer (Timer::CANCEL_ON_DESTROY)
{
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::myhello::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .AddConstructor<RoutingProtocol> ()
    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                   TimeValue (Seconds (beaconInv)),
                   MakeTimeAccessor (&RoutingProtocol::HelloInterval),
                   MakeTimeChecker ())
    .AddAttribute ("DataInterval", "Data messages emission interval.",
                   TimeValue (Seconds (dataInv)),
                   MakeTimeAccessor (&RoutingProtocol::DataInterval),
                   MakeTimeChecker ())              
    .AddAttribute ("MaxQueueLen", "Maximum number of packets that we allow a routing protocol to buffer.",
                   UintegerValue (64),
                   MakeUintegerAccessor (&RoutingProtocol::SetMaxQueueLen,
                                         &RoutingProtocol::GetMaxQueueLen),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxQueueTime", "Maximum time packets can be queued (in seconds)",
                   TimeValue (Seconds (30)),
                   MakeTimeAccessor (&RoutingProtocol::SetMaxQueueTime,
                                     &RoutingProtocol::GetMaxQueueTime),
                   MakeTimeChecker ())
    .AddAttribute ("EnableHello", "Indicates whether a hello messages enable.",
                   BooleanValue (isBeacon),
                   MakeBooleanAccessor (&RoutingProtocol::SetHelloEnable,
                                        &RoutingProtocol::GetHelloEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableData", "Indicates whether a data enable.",
                   BooleanValue (isData),
                   MakeBooleanAccessor (&RoutingProtocol::SetHelloEnable,
                                        &RoutingProtocol::GetHelloEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("UniformRv",
                   "Access to the underlying UniformRandomVariable",
                   StringValue ("ns3::UniformRandomVariable"),
                   MakePointerAccessor (&RoutingProtocol::m_uniformRandomVariable),
                   MakePointerChecker<UniformRandomVariable> ())
  ;
  return tid;
}

void
RoutingProtocol::SetMaxQueueLen (uint32_t len)
{
  MaxQueueLen = len;
  m_queue.SetMaxQueueLen (len);
}
void
RoutingProtocol::SetMaxQueueTime (Time t)
{
  MaxQueueTime = t;
  m_queue.SetQueueTimeout (t);
}

RoutingProtocol::~RoutingProtocol ()
{
}

void
RoutingProtocol::DoDispose ()
{
  m_ipv4 = 0;
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator iter =
         m_socketAddresses.begin (); iter != m_socketAddresses.end (); iter++)
    {
      iter->first->Close ();
    }
  m_socketAddresses.clear ();
  Ipv4RoutingProtocol::DoDispose ();
}

void
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
  *stream->GetStream () << "Node: " << m_ipv4->GetObject<Node> ()->GetId () << " Time: " << Simulator::Now ().GetSeconds () << "s ";
  m_neighborTable.Print (stream);
}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uniformRandomVariable->SetStream (stream);
  return 1;
}

void
RoutingProtocol::Start ()
{
  NS_LOG_FUNCTION (this);
  
  if (((m_ipv4->GetObject<Node> ())->GetId ()) == 0 || 1)
  {
    //std::cout << Simulator::Now ().GetSeconds () << " Start::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  }
  

}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header,
                              Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
	//std::cout << Simulator::Now ().GetSeconds () << " RouteOutput::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
	
  NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));
  if (!p)
    {
      return LoopbackRoute (header, oif); // later
    }
  if (m_socketAddresses.empty ())
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
      NS_LOG_LOGIC ("No myhello interfaces");
      Ptr<Ipv4Route> route;
      return route;
    }
  sockerr = Socket::ERROR_NOTERROR;
  Ptr<Ipv4Route> route;
  Ipv4Address dst = header.GetDestination ();
  NeighborTableEntry nt;
  if (m_neighborTable.LookupValidNeighbor (dst, nt))
    {
      route = nt.GetRoute ();
      NS_ASSERT (route != 0);
      NS_LOG_DEBUG ("Exist route to " << route->GetDestination () << " from interface " << route->GetSource ());
      if (oif != 0 && route->GetOutputDevice () != oif)
        {
          NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
          sockerr = Socket::ERROR_NOROUTETOHOST;
          return Ptr<Ipv4Route> ();
        }
      return route;
    }

  // Valid route not found, in this case we return loopback. 
  // Actual route request will be deferred until packet will be fully formed, 
  // routed to loopback, received from loopback and passed to RouteInput (see below)
  uint32_t iif = (oif ? m_ipv4->GetInterfaceForDevice (oif) : -1);
  DeferredRouteOutputTag tag (iif);
  if (!p->PeekPacketTag (tag))
    {
      p->AddPacketTag (tag);
    }
  return LoopbackRoute (header, oif);
}

void
RoutingProtocol::DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header, 
                                      UnicastForwardCallback ucb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << p << header);
  NS_ASSERT (p != 0 && p != Ptr<Packet> ());

  QueueEntry newEntry (p, header, ucb, ecb);
  bool result = m_queue.Enqueue (newEntry);
  if (result)
    {
      NS_LOG_LOGIC ("Add packet " << p->GetUid () << " to queue. Protocol " << (uint16_t) header.GetProtocol ());
    }
}

bool
RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header,
                             Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
                             MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb)
{
	//std::cout << Simulator::Now ().GetSeconds () << " RouteInput::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  NS_LOG_FUNCTION (this << p->GetUid () << header.GetDestination () << idev->GetAddress ());
  if (m_socketAddresses.empty ())
    {
      NS_LOG_LOGIC ("No myhello interfaces");
      return false;
    }
  NS_ASSERT (m_ipv4 != 0);
  NS_ASSERT (p != 0);
  // Check if input device supports IP
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  int32_t iif = m_ipv4->GetInterfaceForDevice (idev);

  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();

  // Deferred route request
  if (idev == m_lo)
    {
      DeferredRouteOutputTag tag;
      if (p->PeekPacketTag (tag))
        {
          DeferredRouteOutput (p, header, ucb, ecb);
          return true;
        }
    }

  // Duplicate of own packet
  if (IsMyOwnAddress (origin))
    return true;

  // MYHELLO is not a multicast routing protocol
  if (dst.IsMulticast ())
    {
      return false; 
    }

  // Broadcast local delivery/forwarding
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()) == iif)
        if (dst == iface.GetBroadcast () || dst.IsBroadcast ())
          {
            Ptr<Packet> packet = p->Copy ();
            if (lcb.IsNull () == false)
              {
                NS_LOG_LOGIC ("Broadcast local delivery to " << iface.GetLocal ());
                lcb (p, header, iif);
                // Fall through to additional processing
              }
            else
              {
                NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
                ecb (p, header, Socket::ERROR_NOROUTETOHOST);
              }
            return true;
          }
    }

  // Unicast local delivery
  if (m_ipv4->IsDestinationAddress (dst, iif))
    {
      if (lcb.IsNull () == false)
        {
          NS_LOG_LOGIC ("Unicast local delivery to " << dst);
          lcb (p, header, iif);
        }
      else
        {
          NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
          ecb (p, header, Socket::ERROR_NOROUTETOHOST);
        }
      return true;
    }
    
    Ptr<Packet> packet=p->Copy();
    
    UdpHeader udpHeader;
    packet->RemoveHeader (udpHeader);

  // Forwarding
  return Forwarding (packet, header, ucb, ecb);
}

bool
RoutingProtocol::Forwarding (Ptr<const Packet> p, const Ipv4Header & header,
                             UnicastForwardCallback ucb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this);
  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();
  m_neighborTable.Purge ();
  NeighborTableEntry toDst;
  if (m_neighborTable.LookupNeighbor (dst, toDst))
    {
      if (toDst.GetFlag () == VALID)
        {
          Ptr<Ipv4Route> route = toDst.GetRoute ();
          NS_LOG_LOGIC (route->GetSource ()<<" forwarding to " << dst << " from " << origin << " packet " << p->GetUid ());

          NeighborTableEntry toOrigin;
          m_neighborTable.LookupNeighbor (origin, toOrigin);
          

          ucb (route, p, header);
          return true;
        }
    }
  NS_LOG_DEBUG ("Drop packet " << p->GetUid () << " because no route to forward it.");
  return false;
}

void
RoutingProtocol::SetBroadcastTimer()
{
  m_brtimer.Cancel ();
  Time t = Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10)));
  Time temp=m_bTable.NextBTime ();
 // std::cout<<"####Curenttime"<<Simulator::Now().GetSeconds()<<"#####NextBTime"<<m_bTable.NextBTime().GetSeconds()<<"\n";
  if(temp!=Time(0))m_brtimer.Schedule (temp+t-Simulator::Now());	
}

void 
RoutingProtocol::SendTimeoutPkt(){
	BTableEntry tempB;
	PTableEntry tempP;
  
	while(m_bTable.BroadcastB(tempB)){
		if(m_pTable.LookupP(tempB.GetPId(),tempP)){
			Ptr<Packet> packet=tempB.GetPacket();
			for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j){

			  Ptr<Socket> socket = j->first;
			  Ipv4InterfaceAddress iface = j->second;
			  Ipv4Address destination;
			  if (iface.GetMask () == Ipv4Mask::GetOnes ())
			  {
				  destination = Ipv4Address ("255.255.255.255");
			  }
			  else
			  { 
				  destination = iface.GetBroadcast ();
			  }
				
				TypeHeader tHeader (MYHELLOTYPE_DATA);
				uint8_t no_nb=m_nbTable.GetSize();
				uint8_t no_pkt=m_pTable.GetSize();
				uint32_t pktPId[MAX_PKTLIST];
				BeaconHeader beaconHeader(iface.GetLocal (), no_nb, no_pkt, pktPId);
				DataHeader dataHeader;
				
				//packet->RemoveHeader (beaconHeader);
				packet->RemoveHeader(dataHeader);
				dataHeader.SetLifeTime(Simulator::Now ()+ Time(Seconds(PKT_EXPIRE)));
				dataHeader.SetSelectNode(m_nbTable.sDeca());
				dataHeader.IncHop();
				
				int nb = std::max(int(no_nb),1);
				int idealHop = coverageRange/maxRange;
				/*int adaptiveHop = std::min(4, int(ceil(125/nb)));
				int newHop = std::max(4,idealHop + adaptiveHop);*/ //Min
				
				int adaptiveHop = std::min(4, int(ceil(42/nb)));
				int newHop =  std::max(3,idealHop + adaptiveHop - 2); // max
				
				int maxHop = std::min(newHop, int(dataHeader.GetMaxHop()));
				
				/*int nb = std::max(int(no_nb),1);
				int idealHop = coverageRange/maxRange;
				int adaptiveHop = std::min(2, 30/nb);
				  
				int newHop = idealHop + adaptiveHop;
				int maxHop = std::min(newHop, int(dataHeader.GetMaxHop()));*/ // Avg
				
				if(!isAdaptiveHopCounter)
					maxHop = int(dataHeader.GetMaxHop());
				
				dataHeader.SetMaxHop(maxHop);
				
				if(dataHeader.GetHop() > dataHeader.GetMaxHop())
				{
					break;
					return;
				}
				
				Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel>();
				Vector originPosition(double(dataHeader.GetX()), double(dataHeader.GetY()), 0);
				if(decaGPS && CalculateDistance(originPosition, MM->GetPosition()) > coverageRange)
				{
					break;
					return;
				}
				
				packet->AddHeader(dataHeader);
				packet->AddHeader(beaconHeader);
				packet->AddHeader(tHeader);
				
				
				std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " RebcWTData " << MM->GetPosition().x 
         << " " << MM->GetPosition().y << " " << packet->GetUid () << " " << dataHeader.GetSelectNode() << " " << dataHeader.GetHop() << " " << dataHeader.GetMaxHop()  << "\n";
				socket->SendTo (packet, 0, InetSocketAddress (destination, MYHELLO_PORT));	
				
				packet->RemoveHeader(tHeader);
				packet->RemoveHeader(beaconHeader);
				//packet->RemoveHeader(dataHeader);
				//packet->AddHeader(dataHeader);
				
				PTableEntry newPEntry (beaconHeader.GetOrigin (), packet->GetUid (), packet, dataHeader.GetLifeTime ());
				m_pTable.Update (newPEntry);
                
                //std::cout << "TIME: " << dataHeader.GetLifeTime() << "\n";
			}
		}
	}
}

void
RoutingProtocol::BroadcastTimerExpire ()
{
  NS_LOG_FUNCTION (this);
  //std::cout << "BC Timer EXP\n";
  SendTimeoutPkt ();
  SetBroadcastTimer();
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);

  if (isBeacon)
    {
      m_htimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
      m_htimer.Schedule (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 100)));
    }
    
    if (isData)
    {
      m_dtimer.SetFunction (&RoutingProtocol::DataTimerExpire, this);
      m_dtimer.Schedule (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 100)));
    }
    
    
  m_brtimer.SetFunction (&RoutingProtocol::BroadcastTimerExpire, this);
  m_brtimer.Schedule (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 100)));
  m_datatimer.SetFunction (&RoutingProtocol::SendDataTimerExpire, this);
  m_datatimer.Schedule (DataInterval + MilliSeconds (m_uniformRandomVariable->GetInteger (0, 250)));
  
  sentBeacon = -1;
  m_bFlag = true;

  m_ipv4 = ipv4;
  
  if (((m_ipv4->GetObject<Node> ())->GetId ()) == 0 || 1)
  {
    std::cout << Simulator::Now ().GetSeconds () << " SetIpv4::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  }
  
  // Create lo route. It is asserted that the only one interface up for now is loopback
  NS_ASSERT (m_ipv4->GetNInterfaces () == 1 && m_ipv4->GetAddress (0, 0).GetLocal () == Ipv4Address ("127.0.0.1"));
  m_lo = m_ipv4->GetNetDevice (0);
  NS_ASSERT (m_lo != 0);
  // Remember lo route
  NeighborTableEntry rt (/*device=*/ m_lo, /*dst=*/ Ipv4Address::GetLoopback (),
                                    /*iface=*/ Ipv4InterfaceAddress (Ipv4Address::GetLoopback (), Ipv4Mask ("255.0.0.0")),
                                    /*next hop=*/ Ipv4Address::GetLoopback (),
                                    /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
  m_neighborTable.AddNeighbor (rt);

  Simulator::ScheduleNow (&RoutingProtocol::Start, this);
}

void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal ());
  
  if (((m_ipv4->GetObject<Node> ())->GetId ()) == 0 || 1)
  {
    //std::cout << Simulator::Now ().GetSeconds () << " NotifyInterfaceUp::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  }
  
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (l3->GetNAddresses (i) > 1)
    {
      NS_LOG_WARN ("MYHELLO does not work with more then one address per each interface.");
    }
  Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    return;
 
  // Create a socket to listen only on this interface
  Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                             UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvMyhello, this));
  socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), MYHELLO_PORT));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->SetAllowBroadcast (true);
  //socket->SetAttribute ("IpTtl", UintegerValue (1));
  m_socketAddresses.insert (std::make_pair (socket, iface));

  // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (),
                                 UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvMyhello, this));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), MYHELLO_PORT));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->SetAllowBroadcast (true);
  //socket->SetAttribute ("IpTtl", UintegerValue (1));
  m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));
  
  // Add local broadcast record to the routing table
  Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
  NeighborTableEntry nt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*iface=*/ iface,
                                    /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
  m_neighborTable.AddNeighbor (nt);
}

void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal ());
  
  if (((m_ipv4->GetObject<Node> ())->GetId ()) == 0 || 1)
  {
    //std::cout << Simulator::Now ().GetSeconds () << " NotifyInterfaceDown::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  }
  
  // Close socket 
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_ipv4->GetAddress (i, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_socketAddresses.erase (socket);
  
  // Close socket
  socket = FindSubnetBroadcastSocketWithInterfaceAddress (m_ipv4->GetAddress (i, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_socketSubnetBroadcastAddresses.erase (socket);
  
  if (m_socketAddresses.empty ())
    {
      NS_LOG_LOGIC ("No myhello interfaces");
      m_htimer.Cancel ();
      m_neighborTable.Clear ();
      return;
    }
  m_neighborTable.DeleteAllNeighborsFromInterface (m_ipv4->GetAddress (i, 0));
}

void
RoutingProtocol::NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << " interface " << i << " address " << address);
  
  if (((m_ipv4->GetObject<Node> ())->GetId ()) == 0 || 1)
  {
    //std::cout << Simulator::Now ().GetSeconds () << " NotifyAddAddress::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  }
  
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (!l3->IsUp (i))
    return;
  if (l3->GetNAddresses (i) == 1)
    {
      Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
      if (!socket)
        {
          if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
            return;
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                     UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvMyhello,this));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          // Bind to any IP address so that broadcasts can be received
          socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), MYHELLO_PORT));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));

          // Add local broadcast record to the routing table
          Ptr<NetDevice> dev = m_ipv4->GetNetDevice (
              m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
          NeighborTableEntry nt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*iface=*/ iface,
                                            /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
          m_neighborTable.AddNeighbor (nt);
        }
    }
  else
    {
      NS_LOG_LOGIC ("MYHELLO does not work with more then one address per each interface. Ignore added address");
    }
}

void
RoutingProtocol::NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this);
  
  if (((m_ipv4->GetObject<Node> ())->GetId ()) == 0 || 1)
  {
   // std::cout << Simulator::Now ().GetSeconds () << " NotifyRemoveAddress::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  }
  
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (address);
  if (socket)
    {
      m_neighborTable.DeleteAllNeighborsFromInterface (address);
      m_socketAddresses.erase (socket);
      Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
      if (l3->GetNAddresses (i))
        {
          Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                     UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvMyhello, this));
          // Bind to any IP address so that broadcasts can be received
          socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), MYHELLO_PORT));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));

          // Add local broadcast record to the routing table
          Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
          NeighborTableEntry nt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*iface=*/ iface,
                                            /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
          m_neighborTable.AddNeighbor (nt);
        }
      if (m_socketAddresses.empty ())
        {
          NS_LOG_LOGIC ("No myhello interfaces");
          m_htimer.Cancel ();
          m_neighborTable.Clear ();
          return;
        }
    }
  else
    {
      NS_LOG_LOGIC ("Remove address not participating in MYHELLO operation");
    }
}

bool
RoutingProtocol::IsMyOwnAddress (Ipv4Address src)
{
  NS_LOG_FUNCTION (this << src);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (src == iface.GetLocal ())
        {
          return true;
        }
    }
  return false;
}

Ptr<Ipv4Route> 
RoutingProtocol::LoopbackRoute (const Ipv4Header & hdr, Ptr<NetDevice> oif) const
{
  NS_LOG_FUNCTION (this << hdr);
  NS_ASSERT (m_lo != 0);
  Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
  rt->SetDestination (hdr.GetDestination ());
  //
  // Source address selection here is tricky.  The loopback route is
  // returned when MYHELLO does not have a route; this causes the packet
  // to be looped back and handled (cached) in RouteInput() method
  // while a route is found. However, connection-oriented protocols
  // like TCP need to create an endpoint four-tuple (src, src port,
  // dst, dst port) and create a pseudo-header for checksumming.  So,
  // MYHELLO needs to guess correctly what the eventual source address
  // will be.
  //
  // For single interface, single address nodes, this is not a problem.
  // When there are possibly multiple outgoing interfaces, the policy
  // implemented here is to pick the first available MYHELLO interface.
  // If RouteOutput() caller specified an outgoing interface, that 
  // further constrains the selection of source address
  //
  std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
  if (oif)
    {
      // Iterate to find an address on the oif device
      for (j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
        {
          Ipv4Address addr = j->second.GetLocal ();
          int32_t interface = m_ipv4->GetInterfaceForAddress (addr);
          if (oif == m_ipv4->GetNetDevice (static_cast<uint32_t> (interface)))
            {
              rt->SetSource (addr);
              break;
            }
        }
    }
  else
    {
      rt->SetSource (j->second.GetLocal ());
    }
  NS_ASSERT_MSG (rt->GetSource () != Ipv4Address (), "Valid MYHELLO source address not found");
  rt->SetGateway (Ipv4Address ("127.0.0.1"));
  rt->SetOutputDevice (m_lo);
  return rt;
}

void
RoutingProtocol::RecvMyhello (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address sender = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiver = m_socketAddresses[socket].GetLocal ();
  NS_LOG_DEBUG ("MYHELLO node " << this << " received a MYHELLO packet from " << sender << " to " << receiver);

  TypeHeader tHeader;
  packet->RemoveHeader (tHeader);
   Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel>();
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("MYHELLO message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return; // drop
    }
    
   //std::cout << "MYHELLO message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << "\n";
  switch (tHeader.Get ())
    {
    case MYHELLOTYPE_HELLO:
      {
		//std::cout << Simulator::Now ().GetSeconds () << " RecvMyhello[" << packet->GetUid () << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
		std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " RecvBeacon " << MM->GetPosition().x 
         << " " << MM->GetPosition().y << " " << packet->GetUid () << " " << sender << "\n";
        ProcessHello (packet);
        break;
      }
    case MYHELLOTYPE_DATA:
      {
		  //std::cout << "DATA" << "\n";
        ProcessData (packet);
        break;
      }
      case MYHELLOTYPE_FLOODING:
      {
		  //std::cout << "FLOODING" << "\n";
        ProcessFlooding (packet);
        break;
      }
    }
    //std::cout << "\n";
}


void
RoutingProtocol::ProcessHello (Ptr<Packet> p)
{
	//std::cout << Simulator::Now ().GetSeconds () << " ProcessHello[" << p->GetUid () << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  //HelloHeader helloHeader;
  //p->RemoveHeader (helloHeader);
  
  BeaconHeader beaconHeader;
  p->RemoveHeader (beaconHeader);
  Time lifetime = Simulator::Now () + Seconds(5);
  
 // std::cout << Simulator::Now ().GetSeconds () << " No.Neighbor=" << static_cast<int>(beaconHeader.GetNoNb ()) << " No.Pkt=" << static_cast<int>(beaconHeader.GetNoPkt ())  << "\n";
  //NeighborTableEntry toNeighbor;
  NbTableEntry toNeighbor;
  if (!m_nbTable.LookupNb (beaconHeader.GetOrigin (), toNeighbor))
    {

      NbTableEntry newEntry (/*Origin*/ beaconHeader.GetOrigin (),
                                              /*No of Nb=*/ beaconHeader.GetNoNb (),
                                               /*lifeTime=*/ lifetime);
      m_nbTable.AddNb (newEntry);
    }
  else
    {
      //toNeighbor.SetLifeTime (std::max (Time (HelloInterval), toNeighbor.GetLifeTime ()));
      toNeighbor.SetNoNb (beaconHeader.GetNoNb ());
      toNeighbor.SetExpireTime (lifetime);
      m_nbTable.Update (toNeighbor);
    }
    
    //Process of Broadcast Acknowledgement Information
	uint8_t noPktNB=beaconHeader.GetNoPkt();//NB
	uint32_t* recPktNB_=beaconHeader.GetPktId();//NB
	uint32_t recPktNB[noPktNB];
	
	for(uint8_t i=0;i<noPktNB;i++){
		recPktNB[i]=recPktNB_[i];
		//std::cout<<recPktNB[i]<<",";
	}
	
	//std::cout << "\n";
	
	uint8_t noPkt=m_pTable.GetSize();
	
	//std::cout << "Size: " << unsigned(noPkt) << "\n";
//	uint32_t* recPkt_=m_pTable.ListPId();
	uint32_t recPkt[MAX_PKTLIST];
	m_pTable.ListPId_c(recPkt);
	//recPkt = m_pTable.ListPId()->copy();

	bool haveAll=true;
	//Check Nb have all of mine?
	if(noPkt>0){
		for(uint8_t i=0;i<noPkt;i++){
			//std::cout << recPktNB_[i] << "\n";
			bool  havePkt=false;
			for(uint8_t j=0;j<noPktNB;j++){
				if(recPktNB[j]==recPkt[i]){
					havePkt=true;
				}
			}	
			//Nb miss this pkt put the pkt to rebroadcast queue
			if(!havePkt){
				//std::cout << "Detected Missed Pkt: "<<recPkt[i]<<"\n";
				 haveAll=false;
				 PTableEntry tempP;
				 BTableEntry tempB;
				 //Pkt have to be stored if not just let Nb miss (Pkt may be expired)
				 if (m_pTable.LookupP (recPkt[i], tempP)){ 
			//		 std::cout<<"Hav Pkt\n";
					 //If pkt is in Broadcast Queue - Don't add it again
					 if (!m_bTable.LookupB (recPkt[i], tempB)){ 		
						Time t = Time (1 * MilliSeconds (m_uniformRandomVariable->GetInteger (0, 250))); 
						BTableEntry newEntry (tempP.GetPId(), tempP.GetPacket(), Simulator::Now ()+t);
						m_bTable.AddB (newEntry);
						SetBroadcastTimer();
						//if(m_bTable.LookupB (recPkt[i], tempB))std::cout<<"Node("<<m_ipv4->GetObject<Node>()->GetId()<<")::Pkt-("<<recPkt[i]<<")Set WatingTime("<<tempB.GetExpireTime().GetSeconds()<<")\n";
					 }
				 }			
			}
		}
	}
	
	//Check myself have all msg that NB have?
	bool meMiss=false;
	if(!haveAll||(noPkt!=noPktNB)){
		for(uint8_t i=0;i<noPktNB;i++){
			bool havePkt_=false;
			for(uint8_t j=0;j<noPkt;j++){
				if(recPktNB[j]==recPkt[i]){
					havePkt_=true;
				}
			}
			if(!havePkt_){
				meMiss=true;
				break;
			}	
		}			
	}
	if(meMiss&&m_bFlag){
		SendHello ();
		m_bFlag = false;
		//std::cout<<"MeMissed\n";
		//m_bFlag=false;
	} 
	//else if(!m_bFlag) m_bFlag = true;
	
  return;
}

void
RoutingProtocol::ProcessFlooding (Ptr<Packet> p)
{
	Ptr<Packet> packet = p->Copy();
	 Ptr<Packet> newPacket = Create<Packet> ();
	 
	 uint8_t no_nb=m_nbTable.GetSize();
			  uint8_t no_pkt=m_pTable.GetSize();
			  uint32_t pktPId[MAX_PKTLIST];
	 
	 
	BeaconHeader beaconHeader;
	  packet->RemoveHeader (beaconHeader);
	  DataHeader dataHeader;
	  packet->RemoveHeader(dataHeader);
	  
	  packet->AddHeader(dataHeader);
	  
	  Vector originPosition(double(dataHeader.GetX()), double(dataHeader.GetY()), 0);
	  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel>();
	  
	 

    PTableEntry tempP;
  
 // std::cout << Simulator::Now ().GetSeconds () << " No.Neighbor=" << static_cast<int>(beaconHeader.GetNoNb ()) << " No.Pkt=" << static_cast<int>(beaconHeader.GetNoPkt ()) << "\n";
  //NeighborTableEntry toNeighbor;

    //m_bTable.DeleteB (packet->GetUid());
  
    if (!m_pTable.LookupP (dataHeader.GetUid(), tempP)){ //deny duplicated packet 

     PTableEntry newPEntry (beaconHeader.GetOrigin (), dataHeader.GetUid(), packet, Time(Seconds(120)));
     m_pTable.AddP (newPEntry);

      std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " RecvFlooding " << MM->GetPosition().x 
         << " " << MM->GetPosition().y << " " << dataHeader.GetUid() << " " << beaconHeader.GetOrigin() << " " << dataHeader.GetHop() << "\n";
	
   }
         
     // std::cout <<  CalculateDistance(test, MM->GetPosition()) << "\n";
         
      dataHeader.IncHop();
      
       if(dataHeader.GetHop() > dataHeader.GetMaxHop())
		return;
		
	   if(floodingGPS && CalculateDistance(originPosition, MM->GetPosition()) > coverageRange)
			return;
      
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
		 {
			 Ptr<Socket> socket = j->first;
			 Ipv4InterfaceAddress iface = j->second;
      
      BeaconHeader newBeaconHeader(iface.GetLocal (), no_nb, no_pkt, pktPId);

			 newPacket->AddHeader (dataHeader);
			 newPacket->AddHeader (newBeaconHeader);
			 TypeHeader tHeader (MYHELLOTYPE_FLOODING);
			 newPacket->AddHeader (tHeader);
			 
			 Ipv4Address destination;
			  if (iface.GetMask () == Ipv4Mask::GetOnes ())
			  {
				  destination = Ipv4Address ("255.255.255.255");
			  }
			  else
			  { 
				  destination = iface.GetBroadcast ();
			  }
			  m_myPkt.push_back((uint32_t)packet->GetUid());
			  
			  Time t = Simulator::Now () + Time (0.05 * MilliSeconds (m_uniformRandomVariable->GetInteger (0, 100)));
			  while(Simulator::Now ()< t);
			  
			  //std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " FloodingData " << MM->GetPosition().x 
         //<< " " << MM->GetPosition().y << " " << dataHeader.GetUid () << " " << dataHeader.GetSelectNode() << " " << dataHeader.GetHop() << "\n";
			 // std::cout << Simulator::Now ().GetSeconds () << " SForwardData[" << dataHeader.GetUid() << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
			  
			  socket->SendTo (newPacket, 0, InetSocketAddress (destination, MYHELLO_PORT));
			  
		  }
}

void
RoutingProtocol::ProcessData (Ptr<Packet> p)
{
  Ptr<Packet> packet = p->Copy();
  Ptr<Packet> packet_= p->Copy();
  Ptr<Packet> newPacket = Create<Packet> ();
  
  BeaconHeader beaconHeader;
  packet->RemoveHeader (beaconHeader);
  DataHeader dataHeader;
  packet->RemoveHeader(dataHeader);
  dataHeader.SetLifeTime(Simulator::Now ()+ Time(Seconds(PKT_EXPIRE)));
  
 // std::cout << "RecvHeader: " << dataHeader.GetLifeTime() << "\n";
  
  packet->AddHeader(dataHeader);
  
  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel>();
  std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " RecvData " << MM->GetPosition().x 
         << " " << MM->GetPosition().y << " " << dataHeader.GetUid() << " " << beaconHeader.GetOrigin() << " " << dataHeader.GetHop() << " " << dataHeader.GetMaxHop() << "\n";
	//std::cout << Simulator::Now ().GetSeconds () << " RecvData[" << dataHeader.GetUid() << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
	//std::cout << Simulator::Now ().GetSeconds () << " ProcessHello[" << p->GetUid () << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  //HelloHeader helloHeader;
  //p->RemoveHeader (helloHeader);
  //dataHeader.IncHop();
    Time lifetime = Seconds(5);
    
  
  PTableEntry tempP;
  
 // std::cout << Simulator::Now ().GetSeconds () << " No.Neighbor=" << static_cast<int>(beaconHeader.GetNoNb ()) << " No.Pkt=" << static_cast<int>(beaconHeader.GetNoPkt ()) << "\n";
  //NeighborTableEntry toNeighbor;

    //m_bTable.DeleteB (packet->GetUid());
  
  if (!m_pTable.LookupP (dataHeader.GetUid(), tempP)){ //deny duplicated packet 

     PTableEntry newPEntry (beaconHeader.GetOrigin (), dataHeader.GetUid(), packet, dataHeader.GetLifeTime ());
     m_pTable.AddP (newPEntry);
	 //if(m_pTable.AddP (newPEntry)) 
	// std::cout << "Add: " <<  dataHeader.GetUid() << " " << dataHeader.GetLifeTime () << "\n";
	// std::cout << "SIZE: " <<  unsigned(m_pTable.GetSize()) <<"\n";

	 //Ipv4Address selectedNode = dataHeader.GetSelectNode();
	 
	 //std::cout << dataHeader.GetSelectNode () << "\n";
	 
	 if(dataHeader.GetHop() > dataHeader.GetMaxHop())
		return;
		
	Vector originPosition(double(dataHeader.GetX()), double(dataHeader.GetY()), 0);
		
	if(decaGPS && CalculateDistance(originPosition, MM->GetPosition()) > coverageRange)
			return;
     
     if(IsMyOwnAddress(dataHeader.GetSelectNode ()))
     {
		 //std::cout << beaconHeader.GetPktId()[0] <<"Selected \n";

		 for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
		 {
			 Ptr<Socket> socket = j->first;
			 Ipv4InterfaceAddress iface = j->second;
			 
			 dataHeader.SetSelectNode(m_nbTable.sDeca());
			 
			  uint8_t no_nb=m_nbTable.GetSize();
			  uint8_t no_pkt=m_pTable.GetSize();
			  uint32_t pktPId[MAX_PKTLIST];
			  
			  /*int nb = std::max(int(no_nb),1);
			  
			  //double factor = round(5/nb + 0.3);
			  
			  int x = ceil(((nb/5)*maxRange) / nb^2) ;   
			  if(x<2)
				x = int(dataHeader.GetMaxHop());
			  
			  int maxHop = std::min(x,int(dataHeader.GetMaxHop()));
			  dataHeader.SetMaxHop(maxHop);*/
			  
			  int nb = std::max(int(no_nb),1);
				int idealHop = coverageRange/maxRange;
				/*int adaptiveHop = std::min(4, int(ceil(125/nb)));
			  
				int newHop = std::max(4,idealHop + adaptiveHop);*/ //Min
				int adaptiveHop = std::min(4, int(ceil(42/nb)));
				int newHop =  std::max(3,idealHop + adaptiveHop - 2); // max
				int maxHop = std::min(newHop, int(dataHeader.GetMaxHop()));
				
				if(!isAdaptiveHopCounter)
					maxHop = int(dataHeader.GetMaxHop());
				
				dataHeader.SetMaxHop(maxHop);
			  
			  BeaconHeader newBeaconHeader(iface.GetLocal (), no_nb, no_pkt, pktPId);
			  
			  dataHeader.IncHop();

			 newPacket->AddHeader (dataHeader);
			 newPacket->AddHeader (newBeaconHeader);
			 TypeHeader tHeader (MYHELLOTYPE_DATA);
			 newPacket->AddHeader (tHeader);
			 
			 Ipv4Address destination;
			  if (iface.GetMask () == Ipv4Mask::GetOnes ())
			  {
				  destination = Ipv4Address ("255.255.255.255");
			  }
			  else
			  { 
				  destination = iface.GetBroadcast ();
			  }
			  m_myPkt.push_back((uint32_t)packet->GetUid());
			  std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " RebcSNData " << MM->GetPosition().x 
         << " " << MM->GetPosition().y << " " << dataHeader.GetUid () << " " << dataHeader.GetSelectNode() << " " << dataHeader.GetHop() << " " << dataHeader.GetMaxHop() << "\n";
			 // std::cout << Simulator::Now ().GetSeconds () << " SForwardData[" << dataHeader.GetUid() << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
			  socket->SendTo (newPacket, 0, InetSocketAddress (destination, MYHELLO_PORT));
			  
			  newPacket->RemoveHeader (tHeader);
			  newPacket->RemoveHeader (beaconHeader);
			  newPacket->RemoveHeader (dataHeader);
				//dataHeader.IncHop();
				newPacket->AddHeader(dataHeader);
			  
			  PTableEntry newPEntry (beaconHeader.GetOrigin (), dataHeader.GetUid (), newPacket, dataHeader.GetLifeTime ());
			  m_pTable.Update (newPEntry);
		  }
	  }
	  else // not selected
	  {
		  m_myPkt.push_back((uint32_t)packet->GetUid());
		  //newPacket.push_back((uint32_t)packet->GetUid());
		 BTableEntry tempB; 
		 if (!m_bTable.LookupB (dataHeader.GetUid(), tempB)){ 
			 Time t = Time (1 * MilliSeconds (m_uniformRandomVariable->GetInteger (0, 250))); 
			BTableEntry newEntry (dataHeader.GetUid(), packet, Simulator::Now ()+ t);
			m_bTable.AddB (newEntry);
		 }
	//	 std::cout << Simulator::Now ().GetSeconds () << " ForwardData[" << dataHeader.GetUid() << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
			SetBroadcastTimer();
	  }
		
    }
  else // old packet
    {
      //toNeighbor.SetLifeTime (std::max (Time (HelloInterval), toNeighbor.GetLifeTime ()));
      //ProcessHello (packet_);	
	  m_bTable.DeleteB (dataHeader.GetUid());
      //std::cout << "Delete Packet: " << dataHeader.GetUid() << "\n";
    }
  return;
}

//Function that work instead Udp - add it in helper 
// |PACKET|DATA_HEADER|BEACON_HEADER|TYPE_HEADER|UDP_HEADER|
void
RoutingProtocol::AddHeaders (Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route){
	Ptr<Packet> packet=p->Copy();
	//bool newBroadcast = false;
	//std::cout<<"AddHeader"<<endl;Call when any packet (beacon, broadcast) is sending
	//Beacon Pkt Just Let it Through - But if not it would be my own broadcast pkt - store it
	
	//std::cout << "ADDDDD";
	if(p->GetUid()!=sentBeacon){
		//std::cout<<"###Sending packet before addHeader("<<&packet<<")::packetSize("<<packet->GetSize()<<")\n";
        PTableEntry tempP;
        UdpHeader udpHeader;
        packet->RemoveHeader(udpHeader);
     
		if (!m_pTable.LookupP (packet->GetUid(), tempP)){ 

			PTableEntry newEntry (source, packet->GetUid(), packet, Simulator::Now ()+ Time(Seconds(PKT_EXPIRE)));
			m_pTable.AddP (newEntry);
			m_myPkt.push_back((uint32_t)packet->GetUid());
			SetPktExpireTime();
			//std::cout<<Simulator::Now ().GetSeconds()<<"::"<<"Node("<<m_ipv4->GetObject<Node>()->GetId()<<")::Sending New Broadcast Pkt-("<<packet->GetUid()<<")::ExpireTime("<<newEntry.GetExpireTime().GetSeconds()<<")"<<endl;
		// newBroadcast = true;
		
	   std::cout<<Simulator::Now ().GetSeconds()<<" Node "<<m_ipv4->GetObject<Node>()->GetId()/*<< " Addr "<<m_ipv4->GetAddress(1,0).GetLocal()*/<<" sendNewBroadcast "<<packet->GetUid()<<" ExpireTime "<<newEntry.GetExpireTime().GetSeconds();
		 
		// std::cout<<Simulator::Now ().GetSeconds()<<" Node "<<m_ipv4->GetObject<Node>()->GetId()<<" sendNewBroadcast "<<packet->GetUid()<<" ExpireTime "<<newEntry.GetExpireTime().GetSeconds()<<endl;
		
		}

		uint32_t pktPId[MAX_PKTLIST];
		uint32_t* pktPIdL=m_pTable.ListPId();
		for(int i=0;i<MAX_PKTLIST;i++)
			pktPId[i]=pktPIdL[i];
			
			u_int8_t newData = 1;
			
			BeaconHeader beaconHeader (m_ipv4->GetAddress(1,0).GetLocal (),  m_nbTable.GetSize(), m_pTable.GetSize(), pktPId);
			DataHeader dataHeader (/*Selected node*/ m_nbTable.sDeca(), newData,  Simulator::Now ()+ Time(Seconds(PKT_EXPIRE)), packet->GetUid(), 1, 7, 0, 0);
			packet->AddHeader (dataHeader);
			packet->AddHeader(beaconHeader);		
			//std::cout<<"::SelectedNode("<<beaconHeader.GetSelectNode()<<")\n";
		
		TypeHeader tHeader (MYHELLOTYPE_DATA);
		packet->AddHeader (tHeader);
		packet->AddHeader(udpHeader);
		
		m_htimer.Cancel ();
		Time t = Time (0.01 * MilliSeconds (m_uniformRandomVariable->GetInteger (0, 100)));
		m_htimer.Schedule (HelloInterval + t);

		//std::cout<<"###Sending packet("<<&packet<<")::packetSize("<<packet->GetSize()<<")\n";
    }

    //for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j){
	//		Ptr<Socket> socket = j->first;
    //        socket->SendTo (packet, 0, InetSocketAddress (Ipv4Address::GetBroadcast(), dPort));			
	//}
	
	m_downTarget (packet, source, destination, protocol, route);
}

void
RoutingProtocol::SetDownTarget (IpL4Protocol::DownTargetCallback callback)
{
  m_downTarget = callback;
}

IpL4Protocol::DownTargetCallback
RoutingProtocol::GetDownTarget (void) const
{
  return m_downTarget;
}

void
RoutingProtocol::SetPktExpireTime(){
	PTableEntry tempP;
	uint32_t pid=m_myPkt.front();
	if(m_pTable.LookupP(pid,tempP)){
		m_dtimer.Cancel();
		//std::cout<<"Source Set Pkt-"<<pid<<"::ExpireTime("<<tempP.GetExpireTime().GetSeconds()<<")\n";
		m_dtimer.Schedule(tempP.GetExpireTime()-Simulator::Now());
	}

}

void
RoutingProtocol::RemoveStorePkt(uint32_t Pid){
   //std::cout<<"Node("<<m_ipv4->GetObject<Node>()->GetId()<<") Recieve Signal To Delete Pkt-"<<Pid<<"\n";
   m_pTable.DeleteP(Pid);
   m_bTable.DeleteB(Pid);
}

void
RoutingProtocol::RemovePktFromAll(uint32_t Pid){
  NodeContainer c = NodeContainer::GetGlobal ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = (*i);
      Ptr<myhello::RoutingProtocol> deca_ = node->GetObject<myhello::RoutingProtocol> ();
      deca_->RemoveStorePkt(Pid);
    }
}

void
RoutingProtocol::SendDataTimerExpire ()
{
	if((m_ipv4->GetObject<Node> ())->GetId () == 0)
		SendData();
		
	m_datatimer.Cancel ();
		//Time t = Time (0.01 * MilliSeconds (m_uniformRandomVariable->GetInteger (0, 300)));
		//m_datatimer.Schedule (DataInterval + t);
}

void
RoutingProtocol::DataTimerExpire ()
{
  //std::cout << "DATA Timer EXP\n";
  if(!m_myPkt.empty()){	
	   uint32_t tempPid=m_myPkt.front();
	   m_myPkt.pop_front();
	   
	   RemovePktFromAll(tempPid);
	   
	   m_dtimer.Cancel();
	   if(!m_myPkt.empty()){
	   //if have another pkt set timer for delete it
			SetPktExpireTime();
	   }
   }
}

void
RoutingProtocol::HelloTimerExpire ()
{
	//std::cout << Simulator::Now ().GetSeconds () << " HelloTimerExpire::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
  NS_LOG_FUNCTION (this);
  SendHello ();
  m_htimer.Cancel ();
  Time t = Time (0.01 * MilliSeconds (m_uniformRandomVariable->GetInteger (0, 100)));
  m_htimer.Schedule (HelloInterval - t);
  m_bFlag = true;
}

void
RoutingProtocol::SendHello ()
{
	
	Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel>();

  if(floodingGPS || floodingHop)
  {
    std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " GPS " << MM->GetPosition().x 
         << " " << MM->GetPosition().y  << " 0 0 0\n";

    return;
  }

  NS_LOG_FUNCTION (this);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      uint8_t no_nb=m_nbTable.GetSize();
      uint8_t no_pkt=m_pTable.GetSize();
      uint32_t pktPId[MAX_PKTLIST];
	  uint32_t* pktPIdL=m_pTable.ListPId();
      for(int i=0;i<MAX_PKTLIST;i++)
		pktPId[i]=pktPIdL[i];
		
	  //if(no_pkt == 0)
		//return;
		
      Ptr<Socket> socket = j->first;
      //socket->SetAllowBroadcast (true);
      //std::cout << socket;
      Ipv4InterfaceAddress iface = j->second;
      //HelloHeader helloHeader (/*origin=*/ iface.GetLocal (),/*lifetime=*/ Time (HelloInterval));
      BeaconHeader beaconHeader(iface.GetLocal (), no_nb, no_pkt, pktPId);
      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader (beaconHeader);
      TypeHeader tHeader (MYHELLOTYPE_HELLO);
      packet->AddHeader (tHeader);
      //InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        { 
          destination = iface.GetBroadcast ();
        }
        
        
        sentBeacon = packet->GetUid ();
         //std::cout << Simulator::Now ().GetSeconds () << " SendHello[" << packet->GetUid () << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
         //if(Simulator::Now ().GetSeconds () > 30 && Simulator::Now ().GetSeconds () < 31)
         //{
         std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " SendBeacon " << MM->GetPosition().x 
         << " " << MM->GetPosition().y << " " << packet->GetUid () << " " << static_cast<int>(no_nb) << " " << static_cast<int>(no_pkt) << "\n";
		 //}
      socket->SendTo (packet, 0, InetSocketAddress (destination, MYHELLO_PORT));
      
      //socket->Connect (remote);
      //socket->Send (packet);
    }
   
}

void
RoutingProtocol::SendData ()
{
  NS_LOG_FUNCTION (this);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
	  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel>();
	  u_int8_t newData = 1;
      uint8_t no_nb=m_nbTable.GetSize();
      uint8_t no_pkt=m_pTable.GetSize();
      uint32_t pktPId[MAX_PKTLIST];
      
      int nb = std::max(int(no_nb),1);
	  int idealHop = coverageRange/maxRange;
		/*int adaptiveHop = std::min(4, int(ceil(125/nb)));
	  
		int maxHop =  std::max(4,idealHop + adaptiveHop);*/
		int adaptiveHop = std::min(4, int(ceil(42/nb)));
		int maxHop =  std::max(1,idealHop + adaptiveHop - 2); // max
      
      /*int nb = std::max(int(no_nb),1);
      int idealHop = coverageRange/maxRange;
      int adaptiveHop = std::min(2, 30/nb);
      
	  int maxHop =  idealHop + adaptiveHop;*/
	  
      
      //Time lifetime = Seconds(30);
	  Ptr<Packet> packet = Create<Packet> ();
      Ptr<Socket> socket = j->first;
      //socket->SetAllowBroadcast (true);
      //std::cout << socket;
      Ipv4InterfaceAddress iface = j->second;
      //HelloHeader helloHeader (/*origin=*/ iface.GetLocal (),/*lifetime=*/ Time (DataInterval));
      DataHeader dataHeader (/*Selected node*/ m_nbTable.sDeca(), newData, Simulator::Now ()+ Time(Seconds(PKT_EXPIRE)), packet->GetUid(), 1, maxHop, MM->GetPosition().x, MM->GetPosition().y);
      BeaconHeader beaconHeader(iface.GetLocal (), no_nb, no_pkt, pktPId);
     
      
      //std::cout << "Header: " << dataHeader.GetLifeTime() << "\n";
      
      TypeHeader tHeader;
      if(floodingHop)
      {
		tHeader.setType(MYHELLOTYPE_FLOODING);
		dataHeader.SetMaxHop(coverageRange/maxRange);
	  }
	  else if(floodingGPS)
	  {
		  tHeader.setType(MYHELLOTYPE_FLOODING);
		  dataHeader.SetMaxHop(999999999);
	  }
	  else if(decaGPS)
	  {
		  tHeader.setType(MYHELLOTYPE_DATA);
		  dataHeader.SetMaxHop(999999999);
	  }
	  else
	  {
		tHeader.setType(MYHELLOTYPE_DATA);
		dataHeader.SetMaxHop(maxHop);
	  }
		
	  //Vector test = MM->GetPosition();
	
	  packet->AddHeader (dataHeader);
      packet->AddHeader (beaconHeader);
      packet->AddHeader (tHeader);
      //InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        { 
          destination = iface.GetBroadcast ();
        }
        
		//std::cout<<Simulator::Now ().GetSeconds()<<" Node "<<m_ipv4->GetObject<Node>()->GetId() <<" Position "<<myPos.x<<" "<<myPos.y << "\n";
	
	    
		if(myPos.x == MM->GetPosition().x && myPos.y == MM->GetPosition().y) 
		{
		  // std::cout << Simulator::Now ().GetSeconds () << " XSendData[" << packet->GetUid () << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
		std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " SendData " << MM->GetPosition().x 
         << " " << MM->GetPosition().y << " " << packet->GetUid () << " " << dataHeader.GetSelectNode() << " " << dataHeader.GetHop() << " " << dataHeader.GetMaxHop() << "\n";
	    }
		else
		{
		//std::cout << Simulator::Now ().GetSeconds () << " SendData[" << packet->GetUid () << "]::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";
		std::cout << Simulator::Now ().GetSeconds () << " " << (m_ipv4->GetObject<Node> ())->GetId () << " SendData " << MM->GetPosition().x 
         << " " << MM->GetPosition().y << " " << packet->GetUid () << " " << dataHeader.GetSelectNode() << " " << dataHeader.GetHop() << " " << dataHeader.GetMaxHop() << "\n";
	    }
      socket->SendTo (packet, 0, InetSocketAddress (destination, MYHELLO_PORT));
      
      PTableEntry tempP;
      packet->RemoveHeader(tHeader);
      packet->RemoveHeader(beaconHeader);
      packet->RemoveHeader(dataHeader);
      packet->AddHeader (dataHeader);
     
		if (!m_pTable.LookupP (packet->GetUid(), tempP)){ 

			PTableEntry newEntry (beaconHeader.GetOrigin (), packet->GetUid(), packet, Simulator::Now ()+ Time(Seconds(PKT_EXPIRE)));
			m_pTable.AddP (newEntry);
			m_myPkt.push_back((uint32_t)packet->GetUid());
			SetPktExpireTime();
		}
		
		myPos.x = MM->GetPosition().x;
		myPos.y = MM->GetPosition().y;
         
      //socket->Connect (remote);
      //socket->Send (packet);
    }
   
}

void
RoutingProtocol::SendPacketFromQueue (Ipv4Address dst, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this);
  QueueEntry queueEntry;
  while (m_queue.Dequeue (dst, queueEntry))
    {
      DeferredRouteOutputTag tag;
      Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());
      if (p->RemovePacketTag (tag) && 
          tag.GetInterface() != -1 &&
          tag.GetInterface() != m_ipv4->GetInterfaceForDevice (route->GetOutputDevice ()))
        {
          NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
          return;
        }
      UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
      Ipv4Header header = queueEntry.GetIpv4Header ();
      header.SetSource (route->GetSource ());
      ucb (route, p, header);
    }
}

Ptr<Socket>
RoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        return socket;
    }
  Ptr<Socket> socket;
  return socket;
}

Ptr<Socket>
RoutingProtocol::FindSubnetBroadcastSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketSubnetBroadcastAddresses.begin (); j != m_socketSubnetBroadcastAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        return socket;
    }
  Ptr<Socket> socket;
  return socket;
}
}
}