#pragma once
#include <netinet/in.h>

/*
 * 
 * PP1_UDPFileReceiver binds to the portno on any available IPv4 interfaces
 *
 */

class PP1_UDPFileReceiver{
   public:
      PP1_UDPFileReceiver(int portno);
      ~PP1_UDPFileReceiver();

      // Print the valid destination addresses and ports
      void printDestination();

   private:
      int m_portno; // NOTE: Stored in host order not network order (i.e. convert when using for network stuff)
      int m_UDPSocketFD;
      sockaddr_in m_recAddr;
};
