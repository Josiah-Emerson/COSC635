#pragma once
#include <filesystem>
#include <netinet/in.h>
#include "PDU.h"

/*
 *
 * Sends the provided file to the specified address and port 
 *
 */

class PP1_UDPFileSender{
   public:
      // address is a pointer to a string representing an IPv4 address
      // portno is the port on that interface
      // path is a path to the file to be sent
      // The constructor validates the file, and creates hte UDP socket and dest addr
      // creates START datagram ready to be transmitted
      // generates transfer ID
      PP1_UDPFileSender(const char* address, int portno, const std::filesystem::path& path);
      ~PP1_UDPFileSender();

      // TODO: Do we want to return an error?
      void sendFile();

   private: 
      Conv_PDU_Header getConvHeader(Msg_Type msg_type, std::uint64_t seq_num, std::uint16_t payload_len);
   private:
      int m_UDPSocketFD;
      int m_portno; // NOTE: Stored in host format not network
      sockaddr_in m_dest_addr;

      char* m_START_PDU = nullptr;
      std::size_t m_START_PDU_len = 0;
      std::uint32_t m_transferID;

      std::filesystem::path m_path;
};
