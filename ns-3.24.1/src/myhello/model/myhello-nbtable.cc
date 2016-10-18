#include "myhello-nbtable.h"
#include <algorithm>
#include <iomanip>
#include "ns3/simulator.h"
#include "ns3/log.h"



NS_LOG_COMPONENT_DEFINE ("MyhelloNeighborTable");

namespace ns3
{
namespace myhello
{


/*
 The Neighbor Table Entry
 */

NbTableEntry::NbTableEntry(Ipv4Address origin, uint8_t noNb, Time expireTime)
{
	m_origin = origin;
	m_noNb = noNb;
	m_expireTime = expireTime;
}

NbTableEntry::NbTableEntry()
{
}

/*
 The Neighbor Table
 */

bool
NbTable::LookupNb (Ipv4Address id, NbTableEntry & nt)
{
  NS_LOG_FUNCTION (this << id);
  Purge ();
  if (m_nbEntry.empty ()){
      NS_LOG_LOGIC ("Neighbor to " << id << " not found; m_nbEntry is empty");
      return false;
  }
  //Looking for neighbor entry with ip == id
  std::map<Ipv4Address, NbTableEntry>::const_iterator i = m_nbEntry.find (id);
  if (i == m_nbEntry.end ()){
      NS_LOG_LOGIC ("Neighbor to " << id << " not found");
      return false;
  }
  nt = i->second;
  NS_LOG_LOGIC ("Neighbor to " << id << " found");
  return true;
}

bool
NbTable::DeleteNb (Ipv4Address id)
{
  NS_LOG_FUNCTION (this << id);
  Purge ();
  if (m_nbEntry.erase (id) != 0){
      NS_LOG_LOGIC ("Neighbor deletion to " << id << " successful");
      size--;
      return true;
  }
  NS_LOG_LOGIC ("Neighbor deletion to " << id << " not successful");
  return false;
}

bool
NbTable::AddNb (NbTableEntry & nt)
{
  NS_LOG_FUNCTION (this);
  Purge ();
  std::pair<std::map<Ipv4Address, NbTableEntry>::iterator, bool> result = m_nbEntry.insert (std::make_pair (nt.GetOrigin (), nt));
   
  if(result.second){
	  size++;
  }  
  return result.second;
}

bool
NbTable::Update (NbTableEntry & nt)
{ 
  NS_LOG_FUNCTION (this);
  std::map<Ipv4Address, NbTableEntry>::iterator i =
    m_nbEntry.find (nt.GetOrigin ());
  if (i == m_nbEntry.end ())
    {
      NS_LOG_LOGIC ("Neighbor update to " << nt.GetOrigin() << " fails; not found");
      return false;
    } 
  i->second = nt;
  return true;
}

void
NbTable::Purge ()
{
  NS_LOG_FUNCTION (this);
  if (m_nbEntry.empty ())
    return;
    
  for (std::map<Ipv4Address, NbTableEntry>::iterator i = m_nbEntry.begin (); i != m_nbEntry.end ();){
      if (i->second.GetExpireTime () <= Simulator::Now ()){
		  std::map<Ipv4Address, NbTableEntry>::iterator tmp = i;
          ++i;
          m_nbEntry.erase (tmp);
          size--;
      }else{
		  ++i;  
	  }
          
    }
}

Ipv4Address
NbTable::sDeca(){
	uint8_t maxnb=0;
	Ipv4Address maxNb=Ipv4Address("0.0.0.0");
	Purge();
	for (std::map<Ipv4Address, NbTableEntry>::iterator i = m_nbEntry.begin ();
	i != m_nbEntry.end ();i++){
		if((i->second.GetNoNb())>maxnb){
			maxNb=i->second.GetOrigin();
		}
	}
	return maxNb;
}

}
}
