// Program to get a hostname from a given IP address
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
   if(argc < 2){
      fprintf(stderr, "usage %s <IPv4>\n", argv[0]);
      exit(0);
   }

   struct sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   int res = inet_pton(AF_INET, argv[1], &addr.sin_addr);
   if(res != 1){
      if(res == 0){
         fprintf(stderr, "The input '%s' is not a valid IPv4 address\n", argv[1]);
      }else{
         fprintf(stderr, "Programming error inet_pton: %d\n", res);
      }

      exit(0);
   }

   char hbuf[NI_MAXHOST];
   res = getnameinfo((struct sockaddr*)&addr, sizeof(addr), hbuf, sizeof(hbuf), NULL, 0, (NI_NAMEREQD));

   if(res != 0){
      fprintf(stderr, "Error in getnameinfo() call\n");

      switch(res){
         case EAI_AGAIN:
            {
               fprintf(stderr, "Name could not be resolved at this time. Try again later\n");
               break;
            }
         case EAI_FAIL:
            {
               fprintf(stderr, "A nonrecoverable error occurred\n");
               break;
            }
         case EAI_NONAME:
            {
               fprintf(stderr, "The name does not resolve for the supplied arguments. (i.e. host's name cannot be located or was not requested)\n");
               break;
            }
         case EAI_SYSTEM:
            {
               perror("System error: ");
               break;
            }
         case EAI_BADFLAGS:
            {
               fprintf(stderr, "Programmer error: #1\n");
               break;
            }
         case EAI_FAMILY:
            {
               fprintf(stderr, "Programmer error: #2\n");
               break;
            }
         case EAI_OVERFLOW:
            {
               fprintf(stderr, "Programmer error: #3\n");
               break;
            }
         default:
            {
               fprintf(stderr, "WOW!\n");
            }
      }
      exit(0);
   }
   printf("The IPv4 addres %s resolves to %s\n", argv[1], hbuf);
}
