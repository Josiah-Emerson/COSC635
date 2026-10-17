#include "PP1_UDPFileReceiver.h"
#include <iostream>

int main(){
   // TODO: Remove hardcode
   int port = 53317;

   PP1_UDPFileReceiver receiver { port };
   std::cout << "Receiver successfully created\n";

   receiver.printDestination();
   receiver.receiveFile(nullptr);
}
