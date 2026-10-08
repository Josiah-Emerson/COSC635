// Echo server. Listens for messages, and sends reply back to where it came from
// Requires port to listen on 
#include <asm-generic/ioctls.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>

int main(int argc, char* argv[]){
   if(argc < 2){
      fprintf(stderr, "usage %s <portno>\n", argv[0]);
      exit(0);
   }

   // Assume on arg 1
   int port = atoi(argv[1]);
   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if(sockfd == -1){
      fprintf(stderr, "ERROR: Could not create socket\n");
      exit(0);
   }

   struct sockaddr_in servaddr;
   memset(&servaddr, 0, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = htons(port);
   servaddr.sin_addr.s_addr = INADDR_ANY;

   int res = bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
   if(res != 0){
      fprintf(stderr, "ERROR: Could not bind socket\n");
      exit(0);
   }

   res = listen(sockfd, 10);
   if(res != 0){
      fprintf(stderr, "ERROR: Could not listen\n");
      exit(0);
   }

   // TODO: determine and output the address too
   struct ifaddrs* ifaddrs;
   if(getifaddrs(&ifaddrs) != 0){
      fprintf(stderr, "ERROR: Could not getifaddrs");
      exit(0);
   }

   printf("Listening on port %d, and the following ip addresses:\n", port);
   for(struct ifaddrs* ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next){
      if(ifa->ifa_addr == NULL) continue; 
      printf("Interface: %s,   %s (%d)\n\t", ifa->ifa_name, 
            (ifa->ifa_addr->sa_family == AF_PACKET) ? "AF_PACKET" : 
            (ifa->ifa_addr->sa_family == AF_INET) ? "AF_INET" : 
            (ifa->ifa_addr->sa_family == AF_INET6) ? "AF_INET6" : "???",
            ifa->ifa_addr->sa_family);

      if(ifa->ifa_addr->sa_family == AF_INET){
         char hbuf[NI_MAXHOST];
         if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0){
            fprintf(stderr, "ERROR: Could not getnameinfo\n");
         }

         printf("Address: %s\n", hbuf);
      } else {
         printf("Unavailable interface family type.\n");
      }
   }

   freeifaddrs(ifaddrs);
   ifaddrs = NULL;

   struct sockaddr_in clientaddr;
   socklen_t clientaddrsize = sizeof(clientaddr);
   memset(&clientaddr, 0, sizeof(clientaddr));

   printf("Accepting\n");
   fflush(stdout);
   int clientsockfd = accept(sockfd, (struct sockaddr*)&clientaddr, &clientaddrsize);
   printf("After accept\n");
   fflush(stdout);
   if(clientsockfd == -1){
      fprintf(stderr, "ERROR: Could not accept\n");
      exit(0);
   }
   int bufSize;
   res = ioctl(clientsockfd, FIONREAD, &bufSize);
   const int BUF_READ_SIZE = 255;
   char buf[255];
   printf("Reading data: \n");
   fflush(stdout);
   while(bufSize > 0){
      ssize_t sz = recv(clientsockfd, &buf, BUF_READ_SIZE, 0);
      printf("%s", buf);
      fflush(stdout);
      bufSize -= sz;
   }
}
