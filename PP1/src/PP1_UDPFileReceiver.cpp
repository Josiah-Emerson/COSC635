#include "PP1_UDPFileReceiver.h"
#include "Defs.h"
#include "PDU.h"
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

FileReceiveStatus PP1_UDPFileReceiver::receiveFile(std::filesystem::path* path){
   // block and wait for incoming files
   int numPackets = 0;
   while(1){
      char buf[MAX_PDU_SIZE];
      // TODO: Do we care about srcaddr? I think not since we can use transaction ID
      ssize_t sz = recvfrom(m_UDPSocketFD, buf, sizeof(buf), 0, NULL, NULL);
      if(sz < 0){
         // TODO handle
         // TODO: return FAIL
         printf("ERROR: Could not recvfrom\n");
         exit(0);
      }

      if(sz < sizeof(PDU_Header)){
         printf("ERROR: Too few bytes read\n");
         exit(0);
      }

      std::size_t currIdx { 0 };
      PDU_Header header;
      memcpy(&header, &buf[currIdx], sizeof(PDU_Header));
      currIdx += sizeof(PDU_Header);

      Conv_PDU_Header conv_header = PDUHtoConv(header);
      printf("%d: [%s]\n", numPackets++, conv_header.type == Msg_Type::START ? 
            "START" : conv_header.type == Msg_Type::DATA ? 
            "DATA" : conv_header.type == Msg_Type::END ? "END" : "UNKNOWN");
   }
}

/*
FileReceiveStatus PP1_UDPFileReceiver::receiveFile(std::filesystem::path* path){
   // block and wait
   while(1){


      printf("The PDU has the following header values:\n");
      printf("Header version #%d, and is a %s PDU\n", decodeVersion(header.type_version), 
            decodeMsgType(header.type_version) == Msg_Type::START ? "START" : 
            decodeMsgType(header.type_version) == Msg_Type::DATA ? "DATA" : 
            decodeMsgType(header.type_version) == Msg_Type::END ? "END" : "UNKNOWN");
      printf("Transfer id: %d\n", decodeTransferID(&header.transfer_id[0]));
      printf("Seq #%ld with %d valid bytes\n", decodeSequenceNum(&header.sequence_num[0]), header.payload_len);

      // TODO: Branch here on what type of PDU is, but for now assume START
      PDU_START_Header start_header;
      memcpy(&start_header, &buf[currIdx], sizeof(PDU_START_Header));
      currIdx += sizeof(PDU_START_Header);

      char filename[start_header.filename_size + 1];
      char extension[start_header.filename_size + 1];
      memcpy(&filename[0], &buf[currIdx], start_header.filename_size);
      currIdx += start_header.filename_size;
      memcpy(&extension[0], &buf[currIdx], start_header.extension_size);
      currIdx += start_header.extension_size;

      filename[start_header.filename_size] = '\0';
      extension[start_header.extension_size] = '\0';

      printf("The START Header values are:\n");
      printf("Total file size: %ld\n", decodeFileSize(&start_header.fileSize_Bytes[0]));
      printf("Total DATA PDU Count: %ld\n", decodeDATAPDUCnt(&start_header.tot_DATA_PDU_Cnt[0]));
      printf("Filename size: %d\n", start_header.filename_size);
      printf("extension size: %d\n", start_header.extension_size);
      printf("File: %s%s\n", filename, extension);

   }
   return FileReceiveStatus::SUCCESS;
}
*/
