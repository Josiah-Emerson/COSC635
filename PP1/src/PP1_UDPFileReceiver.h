#pragma once
#include <cstdint>
#include <filesystem>
#include <netinet/in.h>

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

   private:
      int m_portno; // NOTE: Stored in host order not network order (i.e. convert when using for network stuff)
      int m_UDPSocketFD;
      sockaddr_in m_recAddr;
};
