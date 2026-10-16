#include "PP1_UDPFileReceiver.h"
#include "Defs.h"
#include "PDU.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
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
   if(m_currentTransfer){
      delete m_currentTransfer;
      m_currentTransfer = nullptr;
   }
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
   // TODO: add timeout
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
      memcpy(&header, &buf[currIdx], sizeof(header));
      currIdx += sizeof(header);

      Conv_PDU_Header conv_header = PDUHtoConv(header);
      if(validatePDU(conv_header, sz) != PDU_ValidationState::VALID){
         // TODO: handle
         printf("ERROR: Invalid PDU state\n");
         exit(0);
      }
      switch(conv_header.type){
         case(Msg_Type::START):
            {
               PDU_START_Header start_header;
               memcpy(&start_header, &buf[currIdx], sizeof(start_header));
               currIdx += sizeof(start_header);

               Conv_PDU_START_Header conv_start_header = PDUSTARTtoConv(start_header);
               char filename[start_header.filename_size + 1];
               char extension[start_header.extension_size + 1];
               memcpy(filename, buf + currIdx, start_header.filename_size);
               currIdx += start_header.filename_size;
               memcpy(extension, buf + currIdx, start_header.extension_size);
               currIdx += start_header.extension_size;

               if(validateStartPDU(conv_start_header, std::string{extension}) != StartHeaderValidationState::VALID){
                  // TODO: handle
                  printf("ERROR: Invalid START PDU \n");
                  exit(0);
               }

               // Valid: dynamically allocate and store metadata
               m_currentTransfer = new CurrentTransferMetadata{
                  .version = conv_header.version,
                  .transferID = conv_header.transferID,
                  .expectedDATAPDUs = conv_start_header.tot_DATA_PDU_Cnt,
                  .expectedFilesize = conv_start_header.fileSize,
                  .filename = filename,
                  .extension = extension,
               };

               // TODO: How we handle errors determines where the byte count increase goes
               m_stats.totalBytes        += sz;
               m_stats.totalPayloadBytes += conv_header.payload_len;

               // Open file for writing by DATA branch
               std::string tmpFile = m_currentTransfer->filename + ".part";
               m_ofstrm = std::ofstream(tmpFile, std::ios::binary);

               if(!m_ofstrm){
                  // TODO: Handle
                  printf("ERROR: could not open file for writing\n");
                  exit(0);
               }
               break;
            }
         case(Msg_Type::DATA):
            {
               if(!m_currentTransfer){ // No valid START has been received
                  // TODO: Update this so that instead of erring out we just discard it and notify
                  printf("ERROR: [DATA] PDU received, but no valid [START] PDU has been received");
                  exit(0);
               }

               if(validateDataPDU(conv_header) != Data_ValidationState::VALID){
                  // TODO: handle
                  // TODO: we can drop repeat seq nums 
                  printf("ERROR: Invalid [DATA] PDU\n");
                  exit(0);
               }

               // TODO: How we handle errors determines where the byte count increase goes
               m_stats.totalBytes        += sz;
               m_stats.totalDATABytes    += conv_header.payload_len;
               m_stats.totalPayloadBytes += conv_header.payload_len;

               // We have a valid DATA PDU
               // Queue for write and mark as received
               queueForWrite(buf + currIdx, conv_header.payload_len, conv_header.sequence_num);
               currIdx += conv_header.payload_len;
               m_currentTransfer->receivedDATAPDUs.insert(conv_header.sequence_num);
               break;
            }
         case(Msg_Type::END):
            {
               if(!m_currentTransfer){
                  // TODO: Handle
                  printf("ERROR: [END] PDU received, but no valid [START] PDU has been received");
                  exit(0);
               }

               if(validateEndPDU(conv_header) != End_ValidationState::VALID){
                     // TODO: handle
                     // TODO: we can drop repeat seq nums 
                     printf("ERROR: Invalid [END] PDU\n");
                     exit(0);
               }

               // TODO: How we handle errors determines where the byte count increase goes
               m_stats.totalBytes        += sz;
               m_stats.totalPayloadBytes += conv_header.payload_len; // TODO/NOTE: There probably should be a check that [END] has no payload

               printf("Wrote %ld of %ld expected DATA PDUs\n", m_currentTransfer->writtenDATAPDUs.size(), m_currentTransfer->expectedDATAPDUs);
               printf("Received %ldB total (including all headers), %ldB of payloads (includes [START] header) and %ldB of %ldB expected\n",
                     m_stats.totalBytes, m_stats.totalPayloadBytes, m_stats.totalDATABytes, m_currentTransfer->expectedFilesize);

               // TODO: Don't just assume file is done
               m_ofstrm.close();
               std::string tmpFile = m_currentTransfer->filename + ".part";
               std::string finalFile = m_currentTransfer->filename + m_currentTransfer->extension;
               std::filesystem::rename(tmpFile, finalFile);
               printf("Your file is accessible at %s\n", finalFile.c_str());

               break;

            }
         default: 
            // TODO: Handle
            printf("ERROR: Unhandled or Unknown message type\n");
            exit(0);
      }
   }
}

PP1_UDPFileReceiver::PDU_ValidationState PP1_UDPFileReceiver::validatePDU(const Conv_PDU_Header& header, ssize_t size){
   // TODO: Add outputs for each of these failpoints? 
   
   if(!SUPPORTED_VERSIONS.count(header.version))
      return PDU_ValidationState::UNSUPPORTED_VERSION;

   if(size > MAX_PDU_SIZE)
      return PDU_ValidationState::INVALID_PDU_SIZE;

   if(header.payload_len > MAX_PDU_PAYLOAD_SIZE)
      return PDU_ValidationState::INVALID_PAYLOAD_LEN;

   if(header.payload_len != (size - sizeof(PDU_Header)))
      return PDU_ValidationState::INVALID_PAYLOAD_LEN;

   return PDU_ValidationState::VALID;
}

PP1_UDPFileReceiver::StartHeaderValidationState PP1_UDPFileReceiver::validateStartPDU(const Conv_PDU_START_Header& start_header, const std::string& extension){
   // TODO: Figure this out
   // NOTE: for now there is no max file size
   if(!SUPPORTED_FILE_TYPES.count(extension))
      return StartHeaderValidationState::UNSUPPORTED_FILE_TYPE;

   return StartHeaderValidationState::VALID;
}

PP1_UDPFileReceiver::Data_ValidationState PP1_UDPFileReceiver::validateDataPDU(const Conv_PDU_Header& header){
   // TODO: Output?
   // NOTE: This function should only be called in one place, but just in case the caller doesn't check nullptr...
   assert(m_currentTransfer && "Called validateDATAPDU without m_currentTransfer");

   if(header.transferID != m_currentTransfer->transferID)
      return Data_ValidationState::INVALID_T_ID;

   if(header.sequence_num > m_currentTransfer->expectedDATAPDUs)
      return Data_ValidationState::INVALID_SEQ_NUM;

   if(m_currentTransfer->receivedDATAPDUs.count(header.sequence_num))
      return Data_ValidationState::REPEAT_SEQ_NUM;

   return Data_ValidationState::VALID;
}

PP1_UDPFileReceiver::End_ValidationState PP1_UDPFileReceiver::validateEndPDU(const Conv_PDU_Header& header){
   // NOTE: This function should only be called in one place, but just in case the caller doesn't check nullptr...
   assert(m_currentTransfer && "Called validateDATAPDU without m_currentTransfer");

   if(header.transferID != m_currentTransfer->transferID) // [END] not for this transfer
      return End_ValidationState::INVALID_T_ID;

   return End_ValidationState::VALID;
}

void PP1_UDPFileReceiver::queueForWrite(const char* buf, std::size_t size, std::uint64_t seqNum){
   // First expected SeqNum is 1
   // TODO/NOTE: This function assumes we don't get repeat seqNum since the DATA branch should account for this,
   // but would be good to add a check for that too eventually
   static std::uint64_t expectedSeqNum { 1 };

   if(seqNum == expectedSeqNum){
      printf("Wrote %ld\n", seqNum);
      m_ofstrm.write(buf, size);
      m_currentTransfer->writtenDATAPDUs.insert(seqNum);
      ++expectedSeqNum;

      // Check queue
      for(const auto& e : m_writeQueue){
         printf("In loop for seq num: %ld\n", e.first);
         // Should happen linearly. We should not be in a state where the first queued element 
         // is lower than the expectedSeqNum
         if(e.first != expectedSeqNum)
            break;

         if(e.first < expectedSeqNum){ // TODO: Remove this check
            printf("ERROR: Got ourselves into a bad queue state\n");
            exit(0);
         }
         printf("Received and wrote queued packet #%ld\n", e.first);
         m_ofstrm.write(e.second.first, e.second.second);
         m_currentTransfer->writtenDATAPDUs.insert(seqNum);
         delete[] e.second.first; // free memory
         m_writeQueue.erase(e.first);
         ++expectedSeqNum;
      }
      return;
   }

   printf("Received [DATA] packet #%ld but expected %ld\n", seqNum, expectedSeqNum);

   // Not ready for this packet yet; add to queue
   char* tmpBuf = new char[size]; // dynamically allocate: need to make sure we delete it
   m_writeQueue.insert({seqNum, {tmpBuf, size}});
}

