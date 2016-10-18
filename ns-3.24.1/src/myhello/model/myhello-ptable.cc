#include "myhello-ptable.h"
#include <algorithm>
#include <iomanip>
#include "ns3/simulator.h"
#include "ns3/log.h"


NS_LOG_COMPONENT_DEFINE ("MyhelloPacketTable");

namespace ns3
{
namespace myhello
{


/*
 Packet Table Entry
 */

PTableEntry::PTableEntry(Ipv4Address origin, uint32_t pid, Ptr<Packet> p,  Time expireTime)
{
	m_origin = origin;
	m_pId = pid;
	m_packet = p->Copy();
	m_expireTime = expireTime;
}

PTableEntry::PTableEntry()
{
}

/*
 The Neighbor Table
 */

bool
PTable::LookupP (uint32_t pId, PTableEntry & nt)
{
  Purge ();
  if (m_pEntry.empty ()){
      return false;
  }
  //Looking for neighbor entry with ip == id
  std::map<uint32_t, PTableEntry>::const_iterator i = m_pEntry.find (pId);
  if (i == m_pEntry.end ()){
      return false;
  }
  nt = i->second;
  return true;
}

bool
PTable::DeleteP (uint32_t pId)
{
  Purge ();
  if (m_pEntry.erase (pId) != 0){
	  size--;
	  //std::cout<<"Deteled the Pkt-"<<pId<<"\n";
      return true;
  }
  return false;
}

bool
PTable::AddP (PTableEntry & nt)
{
  Purge ();
  std::pair<std::map<uint32_t, PTableEntry>::iterator, bool> 
  result = m_pEntry.insert (std::make_pair (nt.GetPId(), nt));
  
  if(result.second){
	  size++;
  } 
   
  return result.second;
}

bool
PTable::Update (PTableEntry & nt)
{ 
  std::map<uint32_t, PTableEntry>::iterator i =
    m_pEntry.find (nt.GetPId ());
  if (i == m_pEntry.end ())
    {
      return false;
    } 
  i->second = nt;
  return true;
}

void
PTable::Purge ()
{
  if (m_pEntry.empty ())
    return;
    
  for (std::map<uint32_t, PTableEntry>::iterator i = m_pEntry.begin (); i != m_pEntry.end ();){
      if (i->second.GetExpireTime () <= Simulator::Now ()){
		  //std::cout<<"Deteled__ the Pkt-"<<i->second.GetPId()<<"\n";
		  std::map<uint32_t, PTableEntry>::iterator tmp = i;
          ++i;
          size--;
          m_pEntry.erase (tmp);
      }else{
		  ++i;  
	  }
          
    }
}

uint32_t*
PTable::ListPId ()
{
  Purge ();	
  uint32_t tempPList[MAX_PKTLIST];
 // std::cout<<"ListPId Call\n";
  std::map<uint32_t, PTableEntry>::iterator i = m_pEntry.begin ();
  for(int index=0;index<MAX_PKTLIST;index++){
	if(i != m_pEntry.end ()){
		tempPList[index]=i->second.GetPId();
//		std::cout<<tempPList[index]<<"\n";
		i++;
	}else{
		tempPList[index]=0;
	}    
  }

  uint32_t* pointer; 
  pointer = tempPList;
  
 // std::cout<<"Return::"<<pointer<<"\n";
  return pointer;
}

void
PTable::ListPId_c (uint32_t pId[])
{
  Purge ();	
 // std::cout<<"ListPId Call\n";
  std::map<uint32_t, PTableEntry>::iterator i = m_pEntry.begin ();
  for(int index=0;index<MAX_PKTLIST;index++){
	if(i != m_pEntry.end ()){
		pId[index]=i->second.GetPId();
//		std::cout<<pId[index]<<"\n";
		i++;
	}else{
		pId[index]=0;
	}    
  }
}

}
}
