#ifndef MYHELLO_BTABLE_H
#define MYHELLO_BTABLE_H

#include "myhello-packet.h"
#include <stdint.h>
#include <cassert>
#include <map>
#include <sys/types.h>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"
#include "ns3/packet.h"

#define BR_WAIT   1
#define BR_NBMISS 2

namespace ns3  
{
namespace myhello 
{

class BTableEntry
{
public:
  
  BTableEntry(uint32_t pid, Ptr<Packet> p, Time broadcastTime);
  BTableEntry ();
  
  uint32_t GetPId() const {return m_pId;}
  void SetPId(uint8_t a){m_pId=a;}
  Ptr<Packet> GetPacket() const {return m_packet;}
  void SetPacket(Ptr<Packet> p){m_packet=p->Copy();}
  Time GetExpireTime() const {return m_broadcastTime;} 
  void SetExpireTime (Time a) {m_broadcastTime=a;}

private:

  uint32_t m_pId;
  Ptr<Packet> m_packet;
  Time m_broadcastTime;
};

class BTable
{
public:
  
  BTable (){}
  bool AddB (BTableEntry & nt);
  bool DeleteB (uint32_t pId);
  bool LookupB (uint32_t pId, BTableEntry & nt);
  bool Update (BTableEntry & nt);
  bool BroadcastB (BTableEntry & nt);
  Time NextBTime();
  uint8_t GetSize() {return m_bEntry.size();}
  
  void Clear () { m_bEntry.clear (); }
  void Purge ();
  
private:
  std::map<uint32_t, BTableEntry> m_bEntry;

};

}
}

#endif /* DECA_NBTABLE_H */
