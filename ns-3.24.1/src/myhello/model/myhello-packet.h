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
#ifndef MYHELLOPACKET_H
#define MYHELLOPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include "ns3/nstime.h"
#include "ns3/vector.h"

#define MAX_NBLIST 10
#define MAX_PKTLIST 20

namespace ns3 {
namespace myhello {

enum MessageType
{
  MYHELLOTYPE_HELLO  = 1,   //!< MYHELLOTYPE_HELLO
  MYHELLOTYPE_DATA  = 2,  //!< MYHELLOTYPE_DATA
  MYHELLOTYPE_FLOODING  = 3  //!< MYHELLOTYPE_FLOODING
};


/**
* \ingroup myhello
* \brief MYHELLO types
*/
class TypeHeader : public Header
{
public:
  /// c-tor
  TypeHeader (MessageType t = MYHELLOTYPE_HELLO);

  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  void setType(MessageType a){m_type = a;}
  //\}

  /// Return type
  MessageType Get () const { return m_type; }
  /// Check that type if valid
  bool IsValid () const { return m_valid; }
  bool operator== (TypeHeader const & o) const;
private:
  MessageType m_type;
  bool m_valid;
};

std::ostream & operator<< (std::ostream & os, TypeHeader const & h);

/**
* \ingroup myhello
* \brief HELLO Message Format
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     Type      |                   Reserved                    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Originator IP address                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Lifetime                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
class HelloHeader : public Header
{
public:
  /// c-tor
  HelloHeader (Ipv4Address origin = Ipv4Address (), Time lifetime = MilliSeconds (0));
  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  //\}

  ///\name Fields
  //\{
  void SetOrigin (Ipv4Address a) { m_origin = a; }
  //void SetPosition (Vector a) { m_position = a; }
  Ipv4Address GetOrigin () const { return m_origin; }
  //Vector GetPosition () const { return m_position; }
  void SetLifeTime (Time t);
  Time GetLifeTime () const;
  //\}

  bool operator== (HelloHeader const & o) const;
private:
  Ipv4Address     m_origin;           ///< Source IP Address
  uint32_t      m_lifeTime;         ///< Lifetime (in milliseconds)
  //Vector		m_position;
};

std::ostream & operator<< (std::ostream & os, HelloHeader const &);

//#################################################################################
class DataHeader : public Header
{
public:
	DataHeader(Ipv4Address selected, u_int8_t data, Time lifetime, u_int64_t uid, u_int64_t hop, u_int64_t maxhop, u_int64_t x, u_int64_t y);
	DataHeader();
	
	//\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  //\}
	
	void SetData(u_int8_t d){m_data=d;}
        void SetSelectNode (Ipv4Address a){ m_selected=a; }
        void SetUid(u_int64_t x){m_uid=x;}
        void SetHop(u_int64_t x){m_hop=x;}
        void SetMaxHop(u_int64_t x){m_maxhop=x;}
        void IncHop(){m_hop+=1;}
        void SetGPS(u_int64_t x, u_int64_t y){m_x=x; m_y=y;}
        u_int64_t GetUid (){ return m_uid; }
        u_int64_t GetHop (){ return m_hop; }
        u_int64_t GetMaxHop (){ return m_maxhop; }
        u_int64_t GetX (){ return m_x; }
        u_int64_t GetY (){ return m_y; }

        Ipv4Address GetSelectNode () const { return m_selected; }
	u_int8_t GetData(){return m_data;}
        void SetLifeTime (Time t){m_lifeTime = t; }
        Time GetLifeTime (){ return m_lifeTime; }
	
	private:
          Ipv4Address	  m_selected;
		  u_int8_t  m_data;
          Time      m_lifeTime;
          u_int64_t m_uid;
          u_int64_t m_hop;
          u_int64_t m_maxhop;
          u_int64_t m_x;
          u_int64_t m_y;
};
std::ostream & operator<< (std::ostream & os, DataHeader const &);

class BeaconHeader : public Header
{
public:
  /// c-tor
  BeaconHeader (Ipv4Address origin, uint8_t noNb,  uint8_t noPkt, uint32_t pktId[MAX_PKTLIST] );
  BeaconHeader ();
  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  //\}

  ///\name Fields
  //\{
  void SetOrigin (Ipv4Address a) { m_origin = a; }
  Ipv4Address GetOrigin () const { return m_origin; }
  void SetNoPkt(uint8_t a){ m_noPkt = a; }
  uint8_t GetNoPkt() { return m_noPkt; }
  void SetNoNb (uint8_t a) { m_noNb = a; }  
  uint8_t GetNoNb() const { return m_noNb; }
  
  inline void SetPktId (uint32_t a[MAX_PKTLIST])
  {
    for(int i = 0;i < MAX_PKTLIST;i++)
      m_pktId[i] = a[i];
  }
  inline uint32_t* GetPktId ()
  {
    uint32_t* pointer;
    pointer = m_pktId;
    return pointer;
  }
  uint32_t getLastPktId ()
  {
    return m_pktId[0];
  }
  //\}

  bool operator== (BeaconHeader const & o) const;
private:
  Ipv4Address     m_origin;         ///< Source IP Address
  uint8_t         m_noNb;
  uint8_t         m_noPkt;               ///< Number of 1-hop Neighbor
  uint32_t        m_pktId[MAX_PKTLIST];
  
};
std::ostream & operator<< (std::ostream & os, BeaconHeader const &);


}
}
#endif /* MYHELLOPACKET_H */
