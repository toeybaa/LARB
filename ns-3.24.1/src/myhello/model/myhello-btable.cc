#include "myhello-btable.h"
#include <algorithm>
#include <iomanip>
#include "ns3/simulator.h"
#include "ns3/log.h"



NS_LOG_COMPONENT_DEFINE ("DecaBroadcastTable");

namespace ns3
{
namespace myhello
{


/*
 Broadcast Table Entry
 */

BTableEntry::BTableEntry(uint32_t pid, Ptr<Packet> p, Time broadcastTime)
{
	m_pId = pid;
	m_packet = p->Copy();
	m_broadcastTime = broadcastTime;
}

BTableEntry::BTableEntry()
{
}

/*
 Broadcast Table
 */

bool
BTable::LookupB (uint32_t pId, BTableEntry & nt)
{
  if (m_bEntry.empty ()){
      return false;
  }
  //Looking for neighbor entry with ip == id
  std::map<uint32_t, BTableEntry>::const_iterator i = m_bEntry.find (pId);
  if (i == m_bEntry.end ()){
      return false;
  }
  nt = i->second;
  return true;
}

bool
BTable::DeleteB (uint32_t pId)
{
  if (m_bEntry.erase (pId) != 0){
      return true;
  }
  return false;
}

bool
BTable::AddB (BTableEntry & nt)
{
  std::pair<std::map<uint32_t, BTableEntry>::iterator, bool> 
  result = m_bEntry.insert (std::make_pair (nt.GetPId(), nt));
   
  return result.second;
}

bool
BTable::Update (BTableEntry & nt)
{ 
  std::map<uint32_t, BTableEntry>::iterator i =
    m_bEntry.find (nt.GetPId ());
  if (i == m_bEntry.end ())
    {
      return false;
    } 
  i->second = nt;
  return true;
}

void
BTable::Purge ()
{
  if (m_bEntry.empty ())
    return;
    
  for (std::map<uint32_t, BTableEntry>::iterator i = m_bEntry.begin (); i != m_bEntry.end ();){
      if (i->second.GetExpireTime () <= Simulator::Now ()){
		  std::map<uint32_t, BTableEntry>::iterator tmp = i;
          ++i;
          m_bEntry.erase (tmp);
      }else{
		  ++i;  
	  }    
   }
}

bool
BTable::BroadcastB (BTableEntry & nt)
{
  //Purge ();	
  if (m_bEntry.empty ())
    return false;	

  std::map<uint32_t, BTableEntry>::iterator i = m_bEntry.begin ();
  for (std::map<uint32_t, BTableEntry>::iterator i = m_bEntry.begin (); i != m_bEntry.end ();){
      if (i->second.GetExpireTime () <= Simulator::Now ()){
		  std::map<uint32_t, BTableEntry>::iterator tmp = i;
          ++i;
          nt = tmp->second;
          m_bEntry.erase (tmp);
          return true;
      }else{
		  ++i;  
	  }    
   }
  return false;
}

Time
BTable::NextBTime ()
{
  Purge ();	
  if (m_bEntry.empty ())
    return Time(0);	

  Time temp=Simulator::GetMaximumSimulationTime ();
  std::map<uint32_t, BTableEntry>::iterator i = m_bEntry.begin ();
  for (std::map<uint32_t, BTableEntry>::iterator i = m_bEntry.begin (); i != m_bEntry.end ();i++){
      if (i->second.GetExpireTime () < temp){
		  temp=	i->second.GetExpireTime ();
	  }    
   }
  return temp;
}

}
}
