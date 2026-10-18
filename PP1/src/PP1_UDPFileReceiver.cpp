#include "PP1_UDPFileReceiver.h"
#include "Defs.h"
#include "PDU.h"
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include "HumanReadable.h"

PP1_UDPFileReceiver::PP1_UDPFileReceiver(int portno)
   : m_portno { portno }
{
   m_UDPSocketFD = socket(AF_INET, SOCK_DGRAM, 0);
   if(m_UDPSocketFD < 0)
      throw std::runtime_error("Unable to create socket");

   memset(&m_recAddr, 0, sizeof(m_recAddr));
   m_recAddr.sin_family      = AF_INET;
   m_recAddr.sin_port        = htons(m_portno);
   m_recAddr.sin_addr.s_addr = INADDR_ANY;

   if(bind(m_UDPSocketFD, (sockaddr*)&m_recAddr, sizeof(m_recAddr)) != 0){ // ERROR
      if(errno == EADDRINUSE) // The port is what was wrong
         throw std::runtime_error("Invalid port number");

      throw std::runtime_error("Could not bind to a socket");
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

   ifaddrs* ifaddrs;
   if(getifaddrs(&ifaddrs) != 0){ 
      throw std::runtime_error("Unable to getifaddrs");
   }


   for(struct ifaddrs* ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next){
      if(ifa->ifa_addr == NULL) continue;

      if(ifa->ifa_addr->sa_family == AF_INET){
         char hbuf[NI_MAXHOST];
         if(getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0){
            // TODO: Are any of the reasons getnameinfo can fail recoverable?
            throw std::runtime_error("Unable to getnameinfo");
         }

         printf("%s:%d\n", hbuf, m_portno);
      }
   }
}

FileReceiveStatus PP1_UDPFileReceiver::receiveFile(){
   // block and wait for incoming files
   auto timeoutStartPoint = std::chrono::steady_clock::now();
   while(1){
      // Check timeout status; must have received [START]
      if(m_currentTransfer && 
            ((std::chrono::steady_clock::now() - timeoutStartPoint) > TIMEOUT_PERIOD)){
         // TODO: handle properly
         timeout();
         return  FileReceiveStatus::TIMEOUT;
      }

      char buf[MAX_PDU_SIZE];
      // TODO: Do we care about srcaddr? I think not since we can use transaction ID
      ssize_t sz = recvfrom(m_UDPSocketFD, buf, sizeof(buf), MSG_DONTWAIT, NULL, NULL);
      if(sz < 0){
         // Treat failure to read for certain circumstances as essentially a dropped packet
         if(errno == EAGAIN || errno == EWOULDBLOCK || 
            errno == ECONNRESET || errno == ETIMEDOUT) {

            continue;
         }else{
            return FileReceiveStatus::READ_FAILURE;
         }
      }

      if(sz < sizeof(PDU_Header)){
         // TODO: Could return an error but for now ignore
         // Treat this as not one of our packets
         fprintf(stderr, "A packet of size smaller than the PDU Header was received and dropped.\n");
         continue;
      }

      std::size_t currIdx { 0 };
      PDU_Header header;
      memcpy(&header, &buf[currIdx], sizeof(header));
      currIdx += sizeof(header);

      Conv_PDU_Header conv_header = PDUHtoConv(header);
      if(validatePDU(conv_header, sz) != PDU_ValidationState::VALID){
         // TODO: Or error out? For now just notify & drop
         fprintf(stderr, "A packet with an invalid PDU header was dropped\n");
         continue;
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
                  // Treat the loss of a [START] header as non-recoverable error
                  return FileReceiveStatus::ILLFORMED_START;
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

               m_stats.totalBytes += sz;

               // Open file for writing by DATA branch
               m_filePath = m_currentTransfer->filename + ".part";
               m_ofstrm = std::ofstream(m_filePath, std::ios::binary);

               if(!m_ofstrm){
                  return FileReceiveStatus::FILE_OPEN_ERR;
               }

               timeoutStartPoint = std::chrono::steady_clock::now();
               break;
            }
         case(Msg_Type::DATA):
            {
               if(!m_currentTransfer){ // No valid START has been received
                  // Notify user and drop packet
                  fprintf(stderr, "[DATA] PDU received, but no valid [START] PDU has been received. Packet dropped.\n");
                  break;
               }

               if(Data_ValidationState state = validateDataPDU(conv_header); state != Data_ValidationState::VALID){
                  switch(state){
                     case(Data_ValidationState::INVALID_T_ID): // Not from our transfer, drop 
                        fprintf(stderr, "[DATA] PDU received with invalid transfer ID. Packet dropped\n.");
                        break;
                     case(Data_ValidationState::REPEAT_SEQ_NUM):
                        fprintf(stderr, "[DATA] PDU with repeat sequence number (#%ld) received. Packet dropped.\n", conv_header.sequence_num);
                        break;
                     case(Data_ValidationState::INVALID_SEQ_NUM):
                        fprintf(stderr, "[DATA] PDU with an invalid sequence number (#%ld) received. Packet dropped.\n", conv_header.sequence_num);
                        break;
                     default: ;
                  }

                  break; // drop packet
               }

               // We have a valid DATA PDU
               // Queue for write and mark as received
               m_stats.totalBytes        += sz;
               queueForWrite(buf + currIdx, conv_header.payload_len, conv_header.sequence_num);
               currIdx += conv_header.payload_len;
               m_currentTransfer->receivedDATAPDUs.insert(conv_header.sequence_num);

               timeoutStartPoint = std::chrono::steady_clock::now();
               break;
            }
         case(Msg_Type::END):
            {
               if(!m_currentTransfer){
                  fprintf(stderr, "[END] PDU received, but no valid [START] PDU has been received. Packet dropped.\n");
                  break;
               }

               if(validateEndPDU(conv_header) != End_ValidationState::VALID){
                  // NOTE: For now the only bad END state we check is transfer ID, in this case just drop it
                  fprintf(stderr, "[END] PDU received with an invalid transfer ID. Packet dropped.\n");
                  break; // drop packet
               }

               m_stats.totalBytes        += sz;
               m_receivedEnd = true;

               timeoutStartPoint = std::chrono::steady_clock::now();
               break;
            }
         default: 
            fprintf(stderr, "PDU of message type [UNKNOWN] received. Packet dropped.\n");
      } // switch

      if(transferComplete()){
         finalize();
         return FileReceiveStatus::SUCCESS;
      }
   } // while
}

void PP1_UDPFileReceiver::printStats(){
   if(!m_currentTransfer)
      return;

   printf("Transmission Summary:\n");
   std::cout << "A total of " << HumanReadable{m_stats.totalBytes} << " of data was received, including headers\n";
   std::cout << m_stats.bytesWritten << " were downloaded from " << m_currentTransfer->writtenDATAPDUs.size() << " [DATA] PDUs\n";

   // check if missing end packet
   if(!m_receivedEnd){
      printf("Missing %sthe [END] PDU. All other packets are accounted for.\n", 
            m_stats.bytesWritten == m_currentTransfer->expectedFilesize ? "only " : "");
   }
   
   // Any outstanding bytes?
   if(m_stats.bytesWritten != m_currentTransfer->expectedFilesize){
      std::cout << "Missing " << HumanReadable{m_currentTransfer->expectedFilesize - m_stats.bytesWritten } << 
         " from " << m_currentTransfer->expectedDATAPDUs - m_currentTransfer->writtenDATAPDUs.size() << " [DATA] PDUs\n";
   }

   printf("This file is available at %s", m_filePath.c_str());
}

PP1_UDPFileReceiver::PDU_ValidationState PP1_UDPFileReceiver::validatePDU(const Conv_PDU_Header& header, ssize_t size){
   // TODO: Way to log why a packet is invalid
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
   // printf("Expected: %ld, Actual: %ld\n", expectedSeqNum, seqNum);

   if(seqNum == expectedSeqNum){
      // printf("Wrote %ld\n", seqNum);
      m_ofstrm.write(buf, size);
      m_currentTransfer->writtenDATAPDUs.insert(seqNum);
      m_stats.bytesWritten += size;
      ++expectedSeqNum;

      // Check queue
      for(auto it = m_writeQueue.begin(); it != m_writeQueue.end();){
         // Should happen linearly. We should not be in a state where the first queued element 
         // is lower than the expectedSeqNum
         if(it->first != expectedSeqNum){
            ++it;
            break;
         }

         if(it->first < expectedSeqNum){ // TODO: Remove this check
            printf("ERROR: Got ourselves into a bad queue state\n");
            exit(0);
         }
         m_ofstrm.write(it->second.first, it->second.second);
         m_currentTransfer->writtenDATAPDUs.insert(it->first);
         m_stats.bytesWritten += it->second.second;
         delete[] it->second.first; // free memory
         it = m_writeQueue.erase(it); // need to set it equal to next 
         ++expectedSeqNum;
      }
      return;
   }

   // Not ready for this packet yet; add to queue
   char* tmpBuf = new char[size]; // dynamically allocate: need to make sure we delete it
   memcpy(tmpBuf, buf, size);
   m_writeQueue.insert({seqNum, {tmpBuf, size}});
}

bool PP1_UDPFileReceiver::transferComplete() const {
   return m_currentTransfer && 
         (m_stats.bytesWritten == m_currentTransfer->expectedFilesize) &&
         (m_currentTransfer->writtenDATAPDUs.size() == m_currentTransfer->expectedDATAPDUs) && 
         m_receivedEnd;
}

void PP1_UDPFileReceiver::finalize(){
   m_ofstrm.close();
   std::string tmpFile = m_currentTransfer->filename + ".part";
   m_filePath = m_currentTransfer->filename + m_currentTransfer->extension;
   std::filesystem::rename(tmpFile, m_filePath);
}

void PP1_UDPFileReceiver::timeout(){
   // TODO: Document this in the README
   // On a timeout, flush the queue and notify the user that they may not have all of the data
   // NOTE: Special case where only the [END] is missing
   assert(m_currentTransfer && "This function should never be called prior to receival of [START]");

   // flush write queue if any
   for(const auto& e : m_writeQueue){
      m_ofstrm.write(e.second.first, e.second.second);
      m_currentTransfer->writtenDATAPDUs.insert(e.first);
      m_stats.bytesWritten += e.second.second;
      delete[] e.second.first;
   }
   m_writeQueue.clear();
   m_ofstrm.close();
}
