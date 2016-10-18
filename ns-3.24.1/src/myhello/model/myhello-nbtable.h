#ifndef MYHELLO_NBTABLE_H
#define MYHELLO_NBTABLE_H

#include "myhello-packet.h"
#include <stdint.h>
#include <cassert>
#include <map>
#include <sys/types.h>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"

namespace ns3 
{
namespace myhello 
{

class NbTableEntry
{
public:
  
  NbTableEntry (Ipv4Address origin, uint8_t noNb, Time expireTime);
  NbTableEntry ();
  
  Ipv4Address GetOrigin() const {return m_origin;}	  
  void SetOrigin(Ipv4Address a){m_origin=a;}
  uint8_t GetNoNb() const {return m_noNb;}
  void SetNoNb(uint8_t a){m_noNb=a;}
  Time GetExpireTime() const {return m_expireTime;} 
  void SetExpireTime (Time a) {m_expireTime=a;}



private:

  Ipv4Address m_origin;
  u_int8_t m_noNb;
  Time m_expireTime;

  
};

class NbTable
{
public:
  
  NbTable (){size=0;}
  bool AddNb (NbTableEntry & nt);
  bool DeleteNb (Ipv4Address origin);
  bool LookupNb (Ipv4Address dst, NbTableEntry & nt);
  bool Update (NbTableEntry & nt);
  Ipv4Address sDeca();
  inline uint8_t GetSize() {
	  Purge();
	  return m_nbEntry.size();
  }
  
  void Clear () { m_nbEntry.clear (); }
  void Purge ();
 
  
private:
  uint8_t size;
  std::map<Ipv4Address, NbTableEntry> m_nbEntry;


};

}
}

#endif /* MYHELLO_NBTABLE_H */
