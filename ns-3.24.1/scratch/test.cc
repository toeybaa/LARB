//std::cout << Simulator::Now ().GetSeconds () << " HelloTimerExpire::Node-> " << (m_ipv4->GetObject<Node> ())->GetId () << "\n";

#include <fstream>
#include <iostream>
#include <algorithm>
#include <limits>
#include <math.h>       /* ceil */

int main (int argc, char *argv[])
{
	for(int i = 2; i<70; i+=10){
		int nb = std::max(int(i),1);
		int idealHop = 2000/500;
		int adaptiveHop = std::min(4, int(ceil(42/nb)));
	  
		int maxHop =  std::max(1,idealHop + adaptiveHop - 2);
		
		std::cout << i << " " << maxHop << "\n";
	}
      
  return 0;
}

