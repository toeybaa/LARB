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
#include "myhello-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3
{
namespace myhello
{

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t) :
  m_type (t), m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::myhello::TypeHeader")
    .SetParent<Header> ()
    .AddConstructor<TypeHeader> ()
  ;
  return tid;
}

TypeId
TypeHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
TypeHeader::GetSerializedSize () const
{
  return 1;
}

void
TypeHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
TypeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  m_valid = true;
  switch (type)
    {
    case MYHELLOTYPE_HELLO:
    case MYHELLOTYPE_DATA:
    case MYHELLOTYPE_FLOODING:
      {
        m_type = (MessageType) type;
        break;
      }
    default:
      m_valid = false;
    }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TypeHeader::Print (std::ostream &os) const
{
  switch (m_type)
    {
    case MYHELLOTYPE_HELLO:
      {
        os << "HELLO";
        break;
      }
	case MYHELLOTYPE_DATA:
      {
        os << "DATA";
        break;
      }
      case MYHELLOTYPE_FLOODING:
      {
        os << "FLOODING";
        break;
      }
    default:
      os << "UNKNOWN_TYPE";
    }
}

bool
TypeHeader::operator== (TypeHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream & os, TypeHeader const & h)
{
  h.Print (os);
  return os;
}

//-----------------------------------------------------------------------------
// HELLO
//-----------------------------------------------------------------------------

HelloHeader::HelloHeader (Ipv4Address origin, Time lifeTime) :
  m_origin (origin)
{
  m_lifeTime = uint32_t (lifeTime.GetMilliSeconds ());
  //m_position = position;
}

NS_OBJECT_ENSURE_REGISTERED (HelloHeader);

TypeId
HelloHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::myhello::HelloHeader")
    .SetParent<Header> ()
    .AddConstructor<HelloHeader> ()
  ;
  return tid;
}

TypeId
HelloHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
HelloHeader::GetSerializedSize () const
{
  return 8;// + sizeof(m_position);
}

void
HelloHeader::Serialize (Buffer::Iterator i) const
{
  WriteTo (i, m_origin);
  i.WriteHtonU32 (m_lifeTime);
  //WriteTo (i, m_position);
}

uint32_t
HelloHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  ReadFrom (i, m_origin);
  m_lifeTime = i.ReadNtohU32 ();
  //ReadFrom (i, m_position);

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
HelloHeader::Print (std::ostream &os) const
{
  os << " source ipv4 " << m_origin << " lifetime " << m_lifeTime;
}

void
HelloHeader::SetLifeTime (Time t)
{
  m_lifeTime = t.GetMilliSeconds ();
}

Time
HelloHeader::GetLifeTime () const
{
  Time t (MilliSeconds (m_lifeTime));
  return t;
}

bool
HelloHeader::operator== (HelloHeader const & o) const
{
  return (m_origin == o.m_origin && m_lifeTime == o.m_lifeTime);
}

std::ostream &
operator<< (std::ostream & os, HelloHeader const & h)
{
  h.Print (os);
  return os;
}


//-----------------------------------------------------------------------------
// BEACON
//-----------------------------------------------------------------------------

BeaconHeader::BeaconHeader (Ipv4Address origin, uint8_t noNb,  uint8_t noPkt, uint32_t pktId[MAX_PKTLIST]) :
  m_origin (origin)
{
  m_noNb = noNb;
	m_noPkt = noPkt;
    for(int i = 0;i < MAX_PKTLIST;i++)
		m_pktId[i] = pktId[i];
}

BeaconHeader::BeaconHeader ()
{
}

NS_OBJECT_ENSURE_REGISTERED (BeaconHeader);

TypeId
BeaconHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::myhello::BeaconHeader")
    .SetParent<Header> ()
    .AddConstructor<BeaconHeader> ()
  ;
  return tid;
}

TypeId
BeaconHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
BeaconHeader::GetSerializedSize () const
{
  return sizeof(m_origin)+sizeof(m_noNb)+sizeof(m_noPkt)+sizeof(m_pktId);
}

void
BeaconHeader::Serialize (Buffer::Iterator i) const
{
  WriteTo (i, m_origin);
  i.WriteU8 ((uint8_t)m_noNb);
  i.WriteU8 ((uint8_t)m_noPkt);
  for(int j = 0;j < MAX_PKTLIST;j++)
	i.WriteU32((uint32_t)m_pktId[j]);
}

uint32_t
BeaconHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  ReadFrom (i, m_origin);
  m_noNb = i.ReadU8();
  m_noPkt = i.ReadU8();
  for(int j = 0;j < MAX_PKTLIST;j++)
	m_pktId[j] = i.ReadU32();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
BeaconHeader::Print (std::ostream &os) const
{
  os << " source ipv4 " << m_origin << " number of neighbors " << m_noNb << " number of packets "<< m_noPkt;
}

bool
BeaconHeader::operator== (BeaconHeader const & o) const
{
  return (m_origin == o.m_origin && m_noNb == o.m_noNb && m_pktId ==o.m_pktId );
}

std::ostream &
operator<< (std::ostream & os, BeaconHeader const & h)
{
  h.Print (os);
  return os;
}

//-----------------------------------------------------------------------------
// DATA
//-----------------------------------------------------------------------------

DataHeader:: DataHeader(Ipv4Address selected, u_int8_t data, Time t, u_int64_t uid, u_int64_t hop, u_int64_t maxhop, u_int64_t x, u_int64_t y)
{
	m_selected = selected;
	m_data = data;
	m_lifeTime = t;
	m_uid = uid;
	m_hop = hop;
	m_maxhop = maxhop;
	m_x = x;
	m_y = y;
}
DataHeader:: DataHeader()
{
}


NS_OBJECT_ENSURE_REGISTERED (DataHeader);

TypeId
DataHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::deca::DataHeader")
    .SetParent<Header> ()
    .AddConstructor<DataHeader> ()
  ;
  return tid;
}

TypeId
DataHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
DataHeader::GetSerializedSize () const
{
  //return 5;
  return sizeof(m_data)+sizeof(m_selected)+sizeof(m_uid)+sizeof(m_hop)+sizeof(m_maxhop)+sizeof(m_x)+sizeof(m_y);
}

void
DataHeader::Serialize (Buffer::Iterator i) const
{
  WriteTo (i, m_selected);
  i.WriteU8 ((uint8_t)m_data);
  //i.WriteHtonU32 (m_lifeTime);
  i.WriteU64 ((uint64_t)m_uid);
  i.WriteU64 ((uint64_t)m_hop);
  i.WriteU64 ((uint64_t)m_maxhop);
  i.WriteU64 ((uint64_t)m_x);
  i.WriteU64 ((uint64_t)m_y);
}

uint32_t
DataHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  ReadFrom (i, m_selected);
  m_data = i.ReadU8();
  //m_lifeTime = i.ReadNtohU32 ();
  m_uid = i.ReadU64();
  m_hop = i.ReadU64();
  m_maxhop = i.ReadU64();
  m_x = i.ReadU64();
  m_y = i.ReadU64();
  
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}
/*
bool
DataHeader::operator== (DataHeader const & o) const
{
  return (m_selected == o.m_selected && m_data == o.m_data && m_lifeTime == o.m_lifeTime && m_uid == o.m_uid );
}*/

void
DataHeader::Print (std::ostream &os) const
{
  os << " selected node " << m_selected << " Data " << m_data;
}

}
}
