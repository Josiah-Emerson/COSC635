#include "PP1_UDPFileSender.h"
#include "PDU.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>

PP1_UDPFileSender::PP1_UDPFileSender(const char* address, int portno, const std::filesystem::path& path)
   : m_portno { portno }
   , m_path { path }
{
   // TODO: add checks for pathname and extension name length
   // TODO: Add sender validations
   // TODO: Check if file is larger than allowed size... 
   // Although our size is so massive it shouldn't be possible
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

   // Build and store START PDU
   // Create and store our transfer ID
   std::uint8_t temp[3];
   createTransferID(temp);
   m_transferID = decodeTransferID(temp);

   // START_Header
   std::uintmax_t fsz = std::filesystem::file_size(m_path);
   Conv_PDU_START_Header c_s_header{
      .fileSize = fsz,
      .tot_DATA_PDU_Cnt = calculateDATAPDUCnt(fsz),
      .filename_size = static_cast<std::uint8_t>(m_path.stem().string().size()),
      .extension_size = static_cast<std::uint8_t>(m_path.extension().string().size())
   };

   PDU_START_Header start_header = ConvToPDUSTART(c_s_header);
   std::size_t payloadsz = start_header.filename_size + start_header.extension_size; 

   // Reg PDU header
   PDU_Header header = ConvToPDUH(getConvHeader(Msg_Type::START, 0, sizeof(start_header) + payloadsz)); // NOTE: the start header is included in the PDU's payload size

   // payload with filename + ext
   m_START_PDU_len = sizeof(header) + sizeof(start_header) + payloadsz;
   m_START_PDU = (char*)malloc(m_START_PDU_len); // NOTE: Destructor needs to free this if it is not nullptr

   // populate header, start_header, and filename/ext
   memcpy(&m_START_PDU[0], &header, sizeof(header));
   memcpy(&m_START_PDU[sizeof(header)], &start_header, sizeof(start_header));
   memcpy(&m_START_PDU[sizeof(header) + sizeof(start_header)], 
         m_path.stem().c_str(), start_header.filename_size);
   memcpy(&m_START_PDU[sizeof(header) + sizeof(start_header) + start_header.filename_size], 
         m_path.extension().c_str(), start_header.extension_size);
}
PP1_UDPFileSender::~PP1_UDPFileSender(){
   if(m_START_PDU){
      free(m_START_PDU);
      m_START_PDU = nullptr;
   }
}

void PP1_UDPFileSender::sendFile(){
   // TODO: Store this elsewhere?
   // TODO: Track and display the stats
   const int MAX_PAYLOAD_SIZE { 1024 };
   // Send START
   ssize_t sent = sendto(m_UDPSocketFD, &m_START_PDU[0], m_START_PDU_len, 0, (sockaddr*)&m_dest_addr, sizeof(m_dest_addr));

   if(sent < 0){
      // TODO: Handle
      printf("ERROR: sendto\n");
      exit(0);
   }

   std::ifstream istrm (m_path, std::ios::binary);
   if(!istrm){
      // TODO: handle
      printf("ERROR: Could not open file to read\n");
      exit(0);
   }

   // send DATA grams
   Conv_PDU_Header conv_header = getConvHeader(Msg_Type::DATA, 1, 0);
   char buf[sizeof(PDU_Header) + MAX_PAYLOAD_SIZE];
   memset(buf, 0, sizeof(buf));

   // TODO: used for delay sim
   char *delayBuf; // Free dynamically if we use this
   std::size_t dBufsize; // 

   while(istrm.read(&buf[sizeof(PDU_Header)], MAX_PAYLOAD_SIZE) || istrm.gcount() > 0){
      conv_header.payload_len = istrm.gcount(); 
      PDU_Header data_header = ConvToPDUH(conv_header);
      memcpy(buf, &data_header, sizeof(data_header));
      // TODO: Below simulates an out of order packet
      // NOTE: we know we have 129, so lets delay packet 127 until after. 
      // This should mean that 128, 129 get queued and then 127 gets written followed by 128 and 129
      if(conv_header.sequence_num == 127){
         dBufsize = sizeof(data_header) + data_header.payload_len;
         char* tmpBuf = new char[dBufsize];
         memcpy(tmpBuf, buf, dBufsize);
         delayBuf = tmpBuf;
         continue;
      }
      sent = sendto(m_UDPSocketFD, buf, sizeof(data_header) + data_header.payload_len, 0, (sockaddr*)&m_dest_addr, sizeof(m_dest_addr));
      if(sent < 0){
         // TODO: Handle
         printf("ERROR: sendto\n");
         exit(0);
      }

      ++conv_header.sequence_num;
   }
   // TODO: out of order below
   sendto(m_UDPSocketFD, delayBuf, dBufsize, 0, (sockaddr*)&m_dest_addr, sizeof(m_dest_addr));
   delete[] delayBuf;
   printf("Sent %ld DATA packets\n", conv_header.sequence_num - 1);

   // send END packet
   // TODO: should seq num be 0 or keep it in order from DATA packets
   conv_header.type = Msg_Type::END;
   conv_header.payload_len = 0;
   PDU_Header end_header = ConvToPDUH(conv_header);
   sent = sendto(m_UDPSocketFD, &end_header, sizeof(end_header), 0, (sockaddr*)&m_dest_addr, sizeof(m_dest_addr));
   if(sent < 0){
      // TODO: Handle
      printf("ERROR: sendto\n");
      exit(0);
   }
}

Conv_PDU_Header PP1_UDPFileSender::getConvHeader(Msg_Type msg_type, std::uint64_t seq_num, std::uint16_t payload_len){
   return {
      .version = 1, // TODO: Can change if we want to
      .type = msg_type,
      .transferID = m_transferID, 
      .sequence_num = seq_num,
      .payload_len = payload_len,
      .reserved = 0
   };
}
