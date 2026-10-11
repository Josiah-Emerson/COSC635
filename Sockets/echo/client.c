#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
   if(argc < 3){
      printf("usage %s hostname port\n", argv[0] );
      exit(0);
   }
   char* addr = argv[1];
   int port = atoi(argv[2]);

   int sockfd = socket(AF_INET, SOCK_STREAM, 0);

   struct sockaddr_in servaddr;
   memset(&servaddr, 0, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = htons(port);

   struct addrinfo hints;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   struct addrinfo* addrs_info;
   if(getaddrinfo(addr, NULL, &hints, &addrs_info) != 0){
      fprintf(stderr, "ERROR: Could not getaddrinfo\n");
      exit(0);
   }

   /*
   // try to connect to each possible?
   struct addrinfo* ai = NULL;
   for(ai = addrs_info; ai != NULL; ai = ai->ai_next){
      if((ai->ai_family != AF_INET) || (ai->ai_socktype != SOCK_STREAM)) continue;

      // servaddr.sin_addr = ((struct sockaddr_in*)(&ai->ai_addr))->sin_addr;
      servaddr.sin_addr = ((struct sockaddr_in*)ai->ai_addr)->sin_addr;
      if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == 0) break;

      perror("Connection error");
   }

   if(!ai){
      fprintf(stderr, "ERROR: Could not connect\n");
      exit(0);
   }

   freeaddrinfo(addrs_info);
   addrs_info = NULL;
   */

   if(inet_pton(AF_INET, addr, &servaddr.sin_addr) != 1){
      perror("ERROR inet_pton");
      exit(0);
   }

   printf("Connecting...\n");
   if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
      perror("ERROR connecting");
      exit(0);
   }
   printf("Done connecting\n");

   struct sockaddr_in addr_check;
   socklen_t addr_check_len = sizeof(addr_check);
   memset(&addr_check, 0, sizeof(addr_check));

   if(getsockname(sockfd, (struct sockaddr*)&addr_check, &addr_check_len) == 0){
      printf("Our address: %s:%d\n",
            inet_ntoa(addr_check.sin_addr),
            ntohs(addr_check.sin_port));
   }

   if(getpeername(sockfd, (struct sockaddr*)&addr_check, &addr_check_len) == 0){ // NOTE: probably not safe to reuse addr_check but oh well
      printf("Peer address: %s:%d\n",
            inet_ntoa(addr_check.sin_addr),
            ntohs(addr_check.sin_port));
   }

   char buffer[] = "Hello, World!\n";
   ssize_t sent = write(sockfd, buffer, sizeof(buffer));
   if(sent < 0){
      fprintf(stderr, "ERROR: Failure in send\n");
   }else{
      printf("Sent %ld bytes\n", sent);
   }

   if(close(sockfd) != 0){
      fprintf(stderr, "ERROR: Failure closing socket\n");
      exit(0);
   }
}
