#include "PP1_UDPFileReceiver.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

void printUsage(const char* prog_name){
   fprintf(stderr, "usage ./%s <portno>\n", prog_name);
}

int main(int argc, char* argv[]){
   if(argc < 2){
      printUsage(argv[0]);
      return 1;
   }

   int port = atoi(argv[1]);
   try{
      PP1_UDPFileReceiver receiver { port };

      printf("The file receiver is currently bound to the following IPv4 addresses:\n");
      receiver.printDestination();

      // TODO: handle
      // TODO: Make prettier
      FileReceiveStatus status = receiver.receiveFile();
      switch(status){
         case(FileReceiveStatus::SUCCESS):
            printf("[SUCCESS] Your file was successfully downloaded!\n\n");
            printf("*****************************************************************\n\n");
            break;
         case(FileReceiveStatus::TIMEOUT):
            printf("[TIMEOUT] The download timed out before all packets were received\n\n");
            printf("[TIMEOUT] Some or all of the file data may be missing\n\n");
            printf("*****************************************************************\n\n");
            break;
         default: // Unrecoverable error
            fprintf(stderr, "[%s] An unrecoverable error was encountered during transmission:\n\n",
                  status == FileReceiveStatus::READ_FAILURE ? "READ FAILURE" : 
                  status == FileReceiveStatus::ILLFORMED_START ? "ILLFORMED START" : 
                  status == FileReceiveStatus::FILE_OPEN_ERR ? "FILE OPEN ERROR" : "???");
            return 1;
      }
      receiver.printStats();
   } catch(const std::runtime_error& e){
      fprintf(stderr, "ERROR: %s\n", e.what());
      return 1;
   }

   return 0;
}
