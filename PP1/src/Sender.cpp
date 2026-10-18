#include <cstdlib>
#include <filesystem>
#include <getopt.h>
#include <unistd.h>
#include "PP1_UDPFileSender.h"


void printUsage(const char* progName){
   fprintf(stderr, "Usage: %s [-i <IPv4 Address>] [-p <portno>] [-f <filename]\n", progName);
}

// TODO: Ensure we accept necessary bits from command line arg
int main(int argc, char* argv[]){
   char* ipAddr = nullptr;
   std::filesystem::path path;
   int opt, portno = -1;

   while((opt = getopt(argc, argv, ":hi:p:f:")) != -1){
      switch(opt){
         case('h'):
            printUsage(argv[0]);
            return 0;
         case('i'):
            ipAddr = optarg;
            break;
         case('p'):
            portno = atoi(optarg);
            break;
         case('f'):
            path = optarg;
            break;
         case(':'):
            fprintf(stderr, "Option -%c requires and argument\n", optopt);
            printUsage(argv[0]);
            return 1;
         case('?'):
            fprintf(stderr, "Uknown option -%c.\n", optopt);
            printUsage(argv[0]);
            return 1;
         default:
            printUsage(argv[0]);
            return 1;
      }
   }

   if(!std::filesystem::exists(path)){
      fprintf(stderr, "The provided file does not exist\n");
      printUsage(argv[0]);
      return 1;
   }

   PP1_UDPFileSender sender { ipAddr, portno, path };
   sender.sendFile();
}
