#ifndef MYHELLO_PTABLE_H
#define MYHELLO_PTABLE_H

#include "myhello-packet.h"
#include <stdint.h>
#include <cassert>
#include <map>
#include <sys/types.h>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"
#include "ns3/packet.h"

namespace ns3 
{
namespace myhello 
{

class PTableEntry
{
public:
  
  PTableEntry(Ipv4Address origin, uint32_t pid, Ptr<Packet> p, Time expireTime);
  PTableEntry ();
  
  Ipv4Address GetOrigin() const {return m_origin;}	  
  void SetOrigin(Ipv4Address a){m_origin=a;}
  uint32_t GetPId() const {return m_pId;}
  void SetPId(uint32_t a){m_pId=a;}
  Ptr<Packet> GetPacket() const {return m_packet;}
  void SetPacket(Ptr<Packet> p){m_packet=p->Copy();}
  Time GetExpireTime() const {return m_expireTime;} 
  void SetExpireTime (Time a) {m_expireTime=a;}



private:

  Ipv4Address m_origin;
  u_int32_t m_pId;
  Ptr<Packet> m_packet;
  Time m_expireTime;
};

class PTable
{
public:
  
  PTable (){size=0;}
  bool AddP (PTableEntry & nt);
  bool DeleteP (uint32_t pId);
  bool LookupP (uint32_t pId, PTableEntry & nt);
  bool Update (PTableEntry & nt);
  uint32_t* ListPId ();
  void ListPId_c(uint32_t pId[]);
  inline uint8_t GetSize() {
	  Purge();
	  return m_pEntry.size();
 }
  
  void Clear () { m_pEntry.clear (); }
  void Purge ();
  
private:
  uint8_t size;
  std::map<uint32_t, PTableEntry> m_pEntry;


};

}
}

#endif /* MYHELLO_NBTABLE_H */
