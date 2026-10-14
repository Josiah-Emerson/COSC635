#pragma once
#include <filesystem>

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
      PP1_UDPFileSender(const char* address, int portno, const std::filesystem::path& path);
      ~PP1_UDPFileSender();

   private:
      int m_UDPSocketFD;
      int m_portno; // NOTE: Stored in host format not network
};
