
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

struct MyPacket		
{
    char type;
    unsigned short seqno;
    int eof_flag;
    char payload[256];
};

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char recv_buf[512];
    char sent_msg[] = "ACK";
    struct MyPacket sent_pkt = {'C', 0, 0, 0, ""};
    int recvlen, payloadlen;
    int LAS = -1;
    int flag = 0;

    struct MyPacket *pkt = malloc(sizeof(struct MyPacket));


    struct sockaddr_in serv_addr, cli_addr;
    socklen_t addrlen = sizeof(cli_addr);
    int n;
    if (argc != 2) 
	{
       fprintf(stderr,"usage %s port\n", argv[0]);
       exit(0);
    }

    /* creat upd socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
       error("ERROR opening socket");

	  /* bind the socket */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

    printf("Waiting on port %d\n", portno);


	   /* create file for writing*/
    FILE *fp;
  	fp = fopen("testercv", "ab");
  	if (NULL == fp)
		error("ERROR opening file");

  	
  	while(recvfrom(sockfd, pkt, sizeof(struct MyPacket), 0, (struct sockaddr *) &cli_addr, &clilen))
  	{
      printf("Feed me data of size %lu, whose sequence number is: %i, and LAS: %i\n", sizeof(*pkt), pkt->seqno, LAS);
      char pkt_type = 'I';
      if (LAS  + 1 == pkt->seqno)
      {
          pkt_type = 'A';
          sent_pkt.type = pkt_type;
          LAS++;
          sent_pkt.seqno = LAS;
          strcpy(sent_pkt.payload,sent_msg);
          printf("Acknowledgment outbound %i\n", sent_pkt.seqno);
          sendto(sockfd, &sent_pkt, sizeof(struct MyPacket), 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
          fputs(pkt->payload, fp);
          if(pkt->eof_flag == 1) 
		{
            printf("Oh no! no more packet\n");
            break;
          }

      } else {
        sent_pkt.type = 'I'; //this runs if you fuck up
        sent_pkt.seqno = LAS;
        strcpy(sent_pkt.payload,sent_msg);
        printf("Ok try again %i\n", sent_pkt.seqno);
        sendto(sockfd, &sent_pkt, sizeof(struct MyPacket), 0, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
      }
  	}

  	fclose(fp);

    close(sockfd);
    return 0;
}
