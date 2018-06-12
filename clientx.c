#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>


//always change tab width damnit

#define WIN = 5;

struct MyPacket		//Design your packet format (fields and possible values)
{
    char type;
    unsigned short seqno;

    
    int eof_flag; 
    

    char payload[256];
};


void error(const char *msg)
{
    perror(msg);
    exit(0);
}


int main(int argc, char *argv[])
{
		
	struct timeval start, stop;
	double secs = 0;
    int sockfd, portno;
    int n = 0;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    socklen_t addrlen = sizeof(serv_addr);
    char buffer[256];
	int LAR = -1;
	int buffer_size = 0;
    int buffer_index = 0;
    
    int repeat_ack = 0;    

    char message[256];

    struct MyPacket pkt_buf[5] = {};
    struct MyPacket pkt = {};
    struct timeval timeout;
    struct MyPacket *pkt_rcvd = malloc(sizeof(struct MyPacket));



    if (argc != 4) {
       fprintf(stderr,"usage %s hostname port filename\n", argv[0]);
       exit(0);
    }


    /* Create a socket */
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    /* fill in the server's address and port*/
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);


    
  
    timeout.tv_usec = 500000;
    int errorid = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (errorid < 0){
        printf("errorid: %d\n", errorid);
        error("ERROR timeout set\n");
    }



 	  /* create file for reading*/

    FILE *fp = fopen("tester", "rb");
    if (fp == NULL)
    {
        error("ERROR open file error");
    }
    gettimeofday(&start, NULL);
    while(1)
    {
        
        if (n > 0)
        {
            int errorid_recv = recvfrom(sockfd, pkt_rcvd, sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, &addrlen);
            if (errorid_recv < 0) {
                
                printf("We have a timeout try again. %i\n", pkt_buf[buffer_index].seqno);
                sendto(sockfd, &pkt_buf[buffer_index], sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
                continue;
            }
           	
            printf("I got the acknowledgement %i\n", pkt_rcvd->seqno);
            if(pkt_rcvd->seqno == LAR + 1) {
                LAR = pkt_rcvd->seqno;
                buffer_size = buffer_size - 1; //shrink buffer
                buffer_index = buffer_index + 1 % 5;
            } else {
           
                printf("Retransmitting. %i, LAR %i\n", pkt_buf[buffer_index].seqno, LAR);
                sendto(sockfd, &pkt_buf[buffer_index], sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
                continue;
            }
        }
        if(buffer_size < 5) {
          message[0] = '\0';
          printf("Next block\n");
          if(feof(fp)) {
              printf("Where are ALL my acknowledgments?\n");
              if(buffer_size == 0) {
                  pkt.eof_flag = 1;
                  pkt.seqno = n; //get final acknowledgment
                  pkt.payload[0] = '\0';
                  if(sendto(sockfd, &pkt, sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 1) error("Sending eof packet fail");
                  break;
              }
              while (buffer_size > 0) {
                  int errorid_recv = recvfrom(sockfd, pkt_rcvd, sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, &addrlen);
                  if (errorid_recv < 0) {
                      
                      printf("Packet resent. %i\n", pkt_buf[buffer_index].seqno);
                      sendto(sockfd, &pkt_buf[buffer_index], sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
                  } else {
                      if(LAR + 1 == pkt_rcvd->seqno) {
                          LAR = pkt_rcvd->seqno;
                          buffer_size = buffer_size - 1;
                          buffer_index = buffer_index + 1 % 5;
                      }
                  }
              }
          } else if (fgets(message, 255, fp) != NULL) { //send it
            pkt.type = 'S';
            pkt.seqno = n;
            n = n + 1;
            strcpy(pkt.payload, message); 
            pkt_buf[(buffer_index + buffer_size) % 5] = pkt;
            buffer_size++;

            printf("HERE IT COMES %i\n", pkt.seqno);
            if(sendto(sockfd, &pkt, sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 1) error("FAILUREEE");

          } else {
            printf("Done reading file\n");
            printf("cmon acknowledgments\n");
            if(buffer_size == 0) {
                pkt.eof_flag = 1;
                pkt.seqno = n;
                pkt.payload[0] = '\0';
                if(sendto(sockfd, &pkt, sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 1) error("You got all the way to the end but the eof file wasn't sent");
                break;
            }
            while (buffer_size > 0) {
                int errorid_recv = recvfrom(sockfd, pkt_rcvd, sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, &addrlen);
                if (errorid_recv < 0) {
                    printf("Retransmitting packet... %i\n", pkt_buf[buffer_index].seqno);
                    sendto(sockfd, &pkt_buf[buffer_index], sizeof(struct MyPacket), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
                } else {
                    if(LAR + 1 == pkt_rcvd->seqno) {
                        LAR = pkt_rcvd->seqno;
                        buffer_size = buffer_size - 1;
                        buffer_index = buffer_index + 1 % 5;
                    }
                }
            }
            break;
          }

        }
    }
	gettimeofday(&stop, NULL);
	///(double) (stop.tv_sec- start.tv_sec)
	secs = (double) (stop.tv_usec - start.tv_usec) / 1000000;
	printf("Time: %f\n", secs);
    close(sockfd);
    return 0;
}
