#include "PP1_UDPFileSender.h"
#include "PDU.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

PP1_UDPFileSender::PP1_UDPFileSender(const char* address, int portno, const std::filesystem::path& path)
   : m_portno { portno }
{
   m_UDPSocketFD = socket(AF_INET, SOCK_DGRAM, 0);

   sockaddr_in dest_addr;
   memset(&dest_addr, 0, sizeof(dest_addr));
   dest_addr.sin_family = AF_INET;
   dest_addr.sin_port = htons(m_portno);

   if(inet_pton(AF_INET, address, &dest_addr.sin_addr) != 1){
      // TODO: actually handle error
      // TODO: Remove related includes
      printf("ERROR: inet_pton\n");
      exit(0);
   }

   printf("Going to send a PDU\n");

   PDU_Header header;
   memset(&header, 0, sizeof(header));
   header.type_version = encodeTypeVersionField(1, Msg_Type::START);
   createTransferID(&header.transfer_id[0]);
   encodeSequenceNumber(0, &header.sequence_num[0]);
   header.payload_len = 0;

   ssize_t sent = sendto(m_UDPSocketFD, &header, sizeof(header), 0, (sockaddr*)&dest_addr, sizeof(dest_addr));

   if(sent < 0){
      // TODO: Handle
      printf("ERROR: sendto\n");
      exit(0);
   }

   printf("Sent %ld bytes\n", sent);
}

PP1_UDPFileSender::~PP1_UDPFileSender(){
}
