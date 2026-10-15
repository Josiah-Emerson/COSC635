#include "PP1_UDPFileSender.h"
#include "PDU.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <sys/socket.h>
#include <arpa/inet.h>

PP1_UDPFileSender::PP1_UDPFileSender(const char* address, int portno, const std::filesystem::path& path)
   : m_portno { portno }
   , m_path { path }
{
   if(!std::filesystem::exists(path)){
      // TODO: handle
      printf("ERROR: file specified by path does not exist\n");
      exit(0);
   }

   m_UDPSocketFD = socket(AF_INET, SOCK_DGRAM, 0);

   memset(&m_dest_addr, 0, sizeof(m_dest_addr));
   m_dest_addr.sin_family = AF_INET;
   m_dest_addr.sin_port = htons(m_portno);

   if(inet_pton(AF_INET, address, &m_dest_addr.sin_addr) != 1){
      // TODO: actually handle error
      // TODO: Remove related includes
      printf("ERROR: inet_pton\n");
      exit(0);
   }

   printf("Going to send a PDU\n");

   PDU_Header header;
   memset(&header, 0, sizeof(header));
   header.type_version = encodeTypeVersionField(1, Msg_Type::DATA);
   createTransferID(&header.transfer_id[0]);
   printf("Transfer ID: %d\n", decodeTransferID(&header.transfer_id[0]));
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

void PP1_UDPFileSender::sendFile(){
}

PDU_START_Header PP1_UDPFileSender::createSTARTHeader(){
   // TODO: Check if file is larger than allowed size... 
   // Although our size is so massive it shouldn't be possible
   PDU_START_Header start_header;
   memset(&start_header, 0, sizeof(start_header));

   std::uintmax_t fsize = std::filesystem::file_size(m_path);
   encodeFileSize(fsize, &start_header.fileSize_Bytes[0]);
}

PP1_UDPFileSender::~PP1_UDPFileSender(){
}
