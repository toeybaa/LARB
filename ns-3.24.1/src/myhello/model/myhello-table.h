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
#ifndef MYHELLO_TABLE_H
#define MYHELLO_TABLE_H

#include <stdint.h>
#include <cassert>
#include <map>
#include <sys/types.h>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"
#include "ns3/net-device.h"
#include "ns3/output-stream-wrapper.h"

namespace ns3 {
namespace myhello {

/**
 * \ingroup myhello
 * \brief Neighbor record states
 */
enum NeighborFlags
{
  VALID = 0,          //!< VALID
  INVALID = 1,        //!< INVALID
};

/**
 * \ingroup myhello
 * \brief Routing table entry
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |             |               Ipv4Route               |         |
  +   LifeTime  +---------------------------------------+   Flag  +
  |             |Destination|Gateway|Source|OutputDevice|         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  
 */
class NeighborTableEntry
{
public:
  /// c-to
  NeighborTableEntry (Ptr<NetDevice> dev = 0,Ipv4Address dst = Ipv4Address (),
                     Ipv4InterfaceAddress iface = Ipv4InterfaceAddress (),
                     Ipv4Address nextHop = Ipv4Address (), Time lifetime = Simulator::Now ());

  ~NeighborTableEntry ();
  
  /// Mark entry as "down" (i.e. disable it)
  void Invalidate (Time badLinkLifetime);
  ///\name Fields
  //\{
  Ipv4Address GetDestination () const { return m_ipv4Route->GetDestination (); }
  Ptr<Ipv4Route> GetRoute () const { return m_ipv4Route; }
  void SetRoute (Ptr<Ipv4Route> r) { m_ipv4Route = r; }
  void SetNextHop (Ipv4Address nextHop) { m_ipv4Route->SetGateway (nextHop); }
  Ipv4Address GetNextHop () const { return m_ipv4Route->GetGateway (); }
  void SetOutputDevice (Ptr<NetDevice> dev) { m_ipv4Route->SetOutputDevice (dev); }
  Ptr<NetDevice> GetOutputDevice () const { return m_ipv4Route->GetOutputDevice (); }
  Ipv4InterfaceAddress GetInterface () const { return m_iface; }
  void SetInterface (Ipv4InterfaceAddress iface) { m_iface = iface; }
  void SetLifeTime (Time lt) { m_lifeTime = lt + Simulator::Now (); }
  Time GetLifeTime () const { return m_lifeTime - Simulator::Now (); }
  void SetFlag (NeighborFlags flag) { m_flag = flag; }
  NeighborFlags GetFlag () const { return m_flag; }
  //\}

  /**
   * \brief Compare destination address
   * \return true if equal
   */
  bool operator== (Ipv4Address const  dst) const
  {
    return (m_ipv4Route->GetDestination () == dst);
  }
  void Print (Ptr<OutputStreamWrapper> stream) const;

private:
  /**
  * \brief Expiration or deletion time of the route
  *	Lifetime field in the routing table plays dual role --
  *	for an active route it is the expiration time, and for an invalid route
  *	it is the deletion time.
  */
  Time m_lifeTime;
  /** Ip route, include
  *   - destination address
  *   - source address
  *   - next hop address (gateway)
  *   - output device
  */
  Ptr<Ipv4Route> m_ipv4Route;
  /// Output interface address
  Ipv4InterfaceAddress m_iface;
  /// Neighbor flags: valid, invalid or in search
  NeighborFlags m_flag;
  
};

/**
 * \ingroup myhello
 * \brief The Routing table used by MYHELLO protocol
 */
class NeighborTable
{
public:
  /// c-tor
  NeighborTable ();
  /**
   * Add neighbor table entry if it doesn't yet exist in neighbor table
   * \param n neighbor table entry
   * \return true in success
   */
  bool AddNeighbor (NeighborTableEntry & n);
  /**
   * Delete neighbor table entry with destination address dst, if it exists.
   * \param dst destination address
   * \return true on success
   */
  bool DeleteNeighbor (Ipv4Address dst);
  /**
   * Lookup neighbor table entry with destination address dst
   * \param dst destination address
   * \param nt entry with destination address dst, if exists
   * \return true on success
   */
  bool LookupNeighbor (Ipv4Address dst, NeighborTableEntry & nt);
  /// Lookup neighbor in VALID state
  bool LookupValidNeighbor (Ipv4Address dst, NeighborTableEntry & nt);
  /// Update neighbor table
  bool Update (NeighborTableEntry & nt);
  /// Set neighbor table entry flags
  bool SetEntryState (Ipv4Address dst, NeighborFlags state);
  /// Delete all neighbor from interface with address iface
  void DeleteAllNeighborsFromInterface (Ipv4InterfaceAddress iface);
  /// Delete all entries from neighbor table
  void Clear () { m_ipv4AddressEntry.clear (); }
  /// Delete all outdated entries and invalidate valid entry if Lifetime is expired
  void Purge ();
  /// Print neighbor table
  void Print (Ptr<OutputStreamWrapper> stream) const;

private:
  std::map<Ipv4Address, NeighborTableEntry> m_ipv4AddressEntry;
  /// const version of Purge, for use by Print() method
  void Purge (std::map<Ipv4Address, NeighborTableEntry> &table) const;
};

}
}

#endif /* MYHELLO_TABLE_H */
