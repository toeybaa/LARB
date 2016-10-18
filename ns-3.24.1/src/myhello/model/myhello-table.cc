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

#include "myhello-table.h"
#include <algorithm>
#include <iomanip>
#include "ns3/simulator.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("MyhelloRoutingTable");

namespace ns3
{
namespace myhello
{

/*
 The Neighbor Table Entry
 */

NeighborTableEntry::NeighborTableEntry (Ptr<NetDevice> dev, Ipv4Address dst,
                                      Ipv4InterfaceAddress iface, Ipv4Address nextHop, Time lifetime) :
  m_lifeTime (lifetime + Simulator::Now ()), m_iface (iface), m_flag (VALID)
{
  m_ipv4Route = Create<Ipv4Route> ();
  m_ipv4Route->SetDestination (dst);
  m_ipv4Route->SetGateway (nextHop);
  m_ipv4Route->SetSource (m_iface.GetLocal ());
  m_ipv4Route->SetOutputDevice (dev);
}

NeighborTableEntry::~NeighborTableEntry ()
{
}

void
NeighborTableEntry::Invalidate (Time badLinkLifetime)
{
  NS_LOG_FUNCTION (this << badLinkLifetime.GetSeconds ());
  if (m_flag == INVALID)
    return;
  m_flag = INVALID;
  m_lifeTime = badLinkLifetime + Simulator::Now ();
}

void
NeighborTableEntry::Print (Ptr<OutputStreamWrapper> stream) const
{
  std::ostream* os = stream->GetStream ();
  *os << m_ipv4Route->GetDestination () << "\t" << m_ipv4Route->GetGateway ()
      << "\t" << m_iface.GetLocal () << "\t";
  switch (m_flag)
    {
    case VALID:
      {
        *os << "UP";
        break;
      }
    case INVALID:
      {
        *os << "DOWN";
        break;
      }
    }
  *os << "\t";
  *os << std::setiosflags (std::ios::fixed) << 
  std::setiosflags (std::ios::left) << std::setprecision (2) <<
  std::setw (14) << (m_lifeTime - Simulator::Now ()).GetSeconds ();
  *os << "\n";
}

/*
 The Neighbor Table
 */

NeighborTable::NeighborTable ()
{
}

bool
NeighborTable::LookupNeighbor (Ipv4Address id, NeighborTableEntry & nt)
{
  NS_LOG_FUNCTION (this << id);
  Purge ();
  if (m_ipv4AddressEntry.empty ())
    {
      NS_LOG_LOGIC ("Neighbor to " << id << " not found; m_ipv4AddressEntry is empty");
      return false;
    }
  std::map<Ipv4Address, NeighborTableEntry>::const_iterator i =
    m_ipv4AddressEntry.find (id);
  if (i == m_ipv4AddressEntry.end ())
    {
      NS_LOG_LOGIC ("Neighbor to " << id << " not found");
      return false;
    }
  nt = i->second;
  NS_LOG_LOGIC ("Neighbor to " << id << " found");
  return true;
}

bool
NeighborTable::LookupValidNeighbor (Ipv4Address id, NeighborTableEntry & nt)
{
  NS_LOG_FUNCTION (this << id);
  if (!LookupNeighbor (id, nt))
    {
      NS_LOG_LOGIC ("Neighbor to " << id << " not found");
      return false;
    }
  NS_LOG_LOGIC ("Neighbor to " << id << " flag is " << ((nt.GetFlag () == VALID) ? "valid" : "not valid"));
  return (nt.GetFlag () == VALID);
}

bool
NeighborTable::DeleteNeighbor (Ipv4Address dst)
{
  NS_LOG_FUNCTION (this << dst);
  Purge ();
  if (m_ipv4AddressEntry.erase (dst) != 0)
    {
      NS_LOG_LOGIC ("Neighbor deletion to " << dst << " successful");
      return true;
    }
  NS_LOG_LOGIC ("Neighbor deletion to " << dst << " not successful");
  return false;
}

bool
NeighborTable::AddNeighbor (NeighborTableEntry & nt)
{
  NS_LOG_FUNCTION (this);
  Purge ();
  std::pair<std::map<Ipv4Address, NeighborTableEntry>::iterator, bool> result =
    m_ipv4AddressEntry.insert (std::make_pair (nt.GetDestination (), nt));
  return result.second;
}

bool
NeighborTable::Update (NeighborTableEntry & nt)
{
  NS_LOG_FUNCTION (this);
  std::map<Ipv4Address, NeighborTableEntry>::iterator i =
    m_ipv4AddressEntry.find (nt.GetDestination ());
  if (i == m_ipv4AddressEntry.end ())
    {
      NS_LOG_LOGIC ("Neighbor update to " << nt.GetDestination () << " fails; not found");
      return false;
    }
  i->second = nt;
  return true;
}

bool
NeighborTable::SetEntryState (Ipv4Address id, NeighborFlags state)
{
  NS_LOG_FUNCTION (this);
  std::map<Ipv4Address, NeighborTableEntry>::iterator i =
    m_ipv4AddressEntry.find (id);
  if (i == m_ipv4AddressEntry.end ())
    {
      NS_LOG_LOGIC ("Neighbor set entry state to " << id << " fails; not found");
      return false;
    }
  i->second.SetFlag (state);
  NS_LOG_LOGIC ("Neighbor set entry state to " << id << ": new state is " << state);
  return true;
}

void
NeighborTable::DeleteAllNeighborsFromInterface (Ipv4InterfaceAddress iface)
{
  NS_LOG_FUNCTION (this);
  if (m_ipv4AddressEntry.empty ())
    return;
  for (std::map<Ipv4Address, NeighborTableEntry>::iterator i =
         m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end ();)
    {
      if (i->second.GetInterface () == iface)
        {
          std::map<Ipv4Address, NeighborTableEntry>::iterator tmp = i;
          ++i;
          m_ipv4AddressEntry.erase (tmp);
        }
      else
        ++i;
    }
}

void
NeighborTable::Purge ()
{
  NS_LOG_FUNCTION (this);
  if (m_ipv4AddressEntry.empty ())
    return;
  for (std::map<Ipv4Address, NeighborTableEntry>::iterator i =
         m_ipv4AddressEntry.begin (); i != m_ipv4AddressEntry.end ();)
    {
      if (i->second.GetLifeTime () < Seconds (0))
        {
          std::map<Ipv4Address, NeighborTableEntry>::iterator tmp = i;
          ++i;
          m_ipv4AddressEntry.erase (tmp);
        }
      else 
        {
          ++i;
        }
    }
}

void
NeighborTable::Purge (std::map<Ipv4Address, NeighborTableEntry> &table) const
{
  NS_LOG_FUNCTION (this);
  if (table.empty ())
    return;
  for (std::map<Ipv4Address, NeighborTableEntry>::iterator i =
         table.begin (); i != table.end ();)
    {
      if (i->second.GetLifeTime () < Seconds (0))
        {
          std::map<Ipv4Address, NeighborTableEntry>::iterator tmp = i;
          ++i;
          table.erase (tmp);
        }
      else 
        {
          ++i;
        }
    }
}

void
NeighborTable::Print (Ptr<OutputStreamWrapper> stream) const
{
  std::map<Ipv4Address, NeighborTableEntry> table = m_ipv4AddressEntry;
  Purge (table);
  *stream->GetStream () << "\nMYHELLO Neighbor table\n"
                        << "Destination\tGateway\t\tInterface\tFlag\tExpire\n";
  for (std::map<Ipv4Address, NeighborTableEntry>::const_iterator i =
         table.begin (); i != table.end (); ++i)
    {
      i->second.Print (stream);
    }
  *stream->GetStream () << "\n";
}

}
}
