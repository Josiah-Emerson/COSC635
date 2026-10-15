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
      PP1_UDPFileSender(const char* address, int portno, const std::filesystem::path& path);
      ~PP1_UDPFileSender();

      // TODO: Do we want to return an error?
      void sendFile();

   private: 
      // [PDU HEADER][PDU_START_HEADER][PAYLOAD]
      //  16                20            N
      // Creates PDU_START_Header based on the file specified in m_path
      PDU_START_Header createSTARTHeader();
   private:
      int m_UDPSocketFD;
      int m_portno; // NOTE: Stored in host format not network
      sockaddr_in m_dest_addr;

      std::filesystem::path m_path;
};
