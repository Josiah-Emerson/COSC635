#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include "PP1_UDPFileSender.h"

struct HumanReadable{
   std::uintmax_t size {};

   private: 
      friend std::ostream& operator<<(std::ostream& os, HumanReadable hr)
       {
           int o{};
           double mantissa = hr.size;
           for (; mantissa >= 1024.; mantissa /= 1024., ++o);
           os << std::ceil(mantissa * 10.) / 10. << "BKMGTPE"[o];
           return o ? os << "B (" << hr.size << ')' : os;
       }
};

// TODO: Ensure we accept necessary bits from command line arg
int main(int argc, char* argv[]){
   std::filesystem::path path { "./resources/example.txt" };
   if(!std::filesystem::exists(path)){
      std::cout << "Does not exist\n";
      return 1;
   }
   // TODO: accept command args 
   std::string addr { "127.0.0.1" };
   int port { 8080 };

   std::cout << "size: " << HumanReadable{std::filesystem::file_size(path)} << '\n';
   std::cout << "Filename: " << path.stem() << '\n';
   std::cout << "Extension: " << path.extension() << '\n';

   PP1_UDPFileSender sender { addr.c_str(), port, path};
   sender.sendFile();
}
