#include "PP1_UDPFileReceiver.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>

PP1_UDPFileReceiver::PP1_UDPFileReceiver(int portno)
   : m_portno { portno }
{
   // TODO: handle errors
   m_UDPSocketFD = socket(AF_INET, SOCK_DGRAM, 0);

   memset(&m_recAddr, 0, sizeof(m_recAddr));
   m_recAddr.sin_family      = AF_INET;
   m_recAddr.sin_port        = htons(m_portno);
   m_recAddr.sin_addr.s_addr = INADDR_ANY;

   if(bind(m_UDPSocketFD, (sockaddr*)&m_recAddr, sizeof(m_recAddr)) != 0){ // ERROR
      // TODO: Actually handle
      printf("ERROR: Could not bind\n");
      exit(0); // Delete the include which is for this
   }
}

// TODO: Actually implement destructor
PP1_UDPFileReceiver::~PP1_UDPFileReceiver(){
}

void PP1_UDPFileReceiver::printDestination(){
   printf("The file receiver is currently bound to the following IPv4 addresses:\n");

   ifaddrs* ifaddrs;
   if(getifaddrs(&ifaddrs) != 0){ // ERROR
      // TODO: Actually handle
      printf("ERROR: Could not getifaddrs\n");
      exit(0);
   }

   for(struct ifaddrs* ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next){
      if(ifa->ifa_addr == NULL) continue;

      if(ifa->ifa_addr->sa_family == AF_INET){
         char hbuf[NI_MAXHOST];
         if(getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0){
            // TODO: Handle 
            // TODO: Is this an error or do we just skip?
            printf("ERROR: Could not getnameinfo\n");
            exit(0);
         }

         printf("%s:%d\n", hbuf, m_portno);
      }
   }
}
