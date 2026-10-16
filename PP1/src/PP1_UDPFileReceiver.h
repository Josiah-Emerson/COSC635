#pragma once
#include "PDU.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <netinet/in.h>
#include <set>
#include <string>

/*
 * 
 * PP1_UDPFileReceiver binds to the portno on any available IPv4 interfaces
 *
 */

// TODO: Any other statuses? 
// For now I can only think of these too
enum class FileReceiveStatus{
   SUCCESS,
   FAIL,
};

class PP1_UDPFileReceiver{
   public:
      PP1_UDPFileReceiver(int portno);
      ~PP1_UDPFileReceiver();

      // Print the valid destination addresses and ports
      void printDestination();

      // NOTE: This function blocks
      // It waits for a START datagram 
      // then attempts to build the file as datagrams flow in 
      // If the return value is SUCCESS, path will contain the path to where that file is
      // If the return value is not a SUCCESS, path will be a nullptr 
      // TODO: May have path point to the path anyways if we want to be able to view part of it
      FileReceiveStatus receiveFile(std::filesystem::path* path);

   private: 
      // Tracking of the transmission stats
      struct Stats{
         std::uintmax_t totalBytes        = 0; // All the bytes received by every datagram
         std::uintmax_t totalPayloadBytes = 0; // Total bytes not including the PDU_Header for each datagram (essentially the DATA payload + (PDU_START_Header + payload of START PDU))
         std::uintmax_t totalDATABytes    = 0; // Total bytes for the payloads on DATA datagram (only the payload on DATA datagrams). Should be the size of the file
      };

      // Contains the necessary metadata to validate a packet once a START packet has been received
      struct CurrentTransferMetadata{
         const int version;
         const std::uint32_t transferID;
         const std::uint64_t expectedDATAPDUs;
         const std::uintmax_t expectedFilesize;

         const std::string filename;
         const std::string extension;

         std::set<std::uint64_t> receivedDATAPDUs; // track which PDUs we have received
         std::set<std::uint64_t> writtenDATAPDUs;  // track which PDUs we have written
      };

      enum class PDU_ValidationState{
         VALID,
         UNSUPPORTED_VERSION,   // version number is not supported
         INVALID_PDU_SIZE,      // size of payload read in is greater than maximum allowed PDU size
         INVALID_PAYLOAD_LEN,   // payload_len > maximum allowed payload size 
         INCORRECT_PAYLOAD_LEN, // Payload size != payload_len 
      };

      // size is the entire size of the PDU read in (including the headers)
      PDU_ValidationState validatePDU(const Conv_PDU_Header& header, ssize_t size);

      enum class StartHeaderValidationState {
         VALID,
         UNSUPPORTED_FILE_TYPE,
         INVALID_FILE_SIZE,
      };

      StartHeaderValidationState validateStartPDU(const Conv_PDU_START_Header& start_header, const std::string& extension);

      enum class Data_ValidationState{
         VALID,
         INVALID_T_ID,
         INVALID_SEQ_NUM,
         REPEAT_SEQ_NUM, // NOTE: we can handle this if needed
      };

      Data_ValidationState validateDataPDU(const Conv_PDU_Header& header);

      enum class End_ValidationState{
         VALID,
         INVALID_T_ID,
      };
      End_ValidationState validateEndPDU(const Conv_PDU_Header& header);

      // Queues a chunk of received data to be written 
      // If the seq_num is the next expected number writes it immediately, 
      // Otherwise copies to the write queue
      void queueForWrite(const char* buf, std::size_t size, std::uint64_t seqNum);
   private:

      int m_portno; // NOTE: Stored in host order not network order (i.e. convert when using for network stuff)
      int m_UDPSocketFD;
      sockaddr_in m_recAddr;

      Stats m_stats { };
      CurrentTransferMetadata* m_currentTransfer = nullptr; // Becomes not nullptr when a START PDU is received
                                                            // Need to make sure this memory is freed in destructor
      std::ofstream m_ofstrm; // writer to the file we want 
      std::map<std::uint64_t, std::pair<char*, std::size_t> > m_writeQueue; // map from seq num to data* and size
                                                                            // NOTE: any removal from the map needs to free the pointer
};
