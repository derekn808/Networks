// Derek Nakamura
// Lab 4
// UDP client
// sends a file 10 bytes at a time as a struct packet
// packet contains a header struct which holds packet length, checksum,
// and sequence or ack number
// Client waits for server to send back a correct ack to ensure that
// packet sent was correct
// Client will only send a new packet once it gets the correct ack
// Client will send a packet with 0 len to server to indicate that the
// file is done sending.
// Has a timeout function that will give a timeout if the server takes
// longer than 10 seonds to send back an ACK.
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <fcntl.h>

struct HEADER
{
	int seq_ack;	//SEQ for data and ACK for Acknowledgement
	int len;		//Length of the data in bytes (zero for ACKs)
	int cksum;		//checksum calculated (by byte)
};

struct PACKET
{
	struct HEADER head;
	char data[10];
};

int csum(char* buff, int buffsize)
{
	int cs=0;
	int i;
	for(i=0; i<buffsize; i++)
	{
		cs^=buff[i];
	}
	return cs;
}

int main(int argc, char *argv[])
{
	int sock, portNum, nBytes, rv, i, chk;
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	struct PACKET p;
	FILE *src;
	int state=0;
	struct HEADER ack;
	struct timeval tv;
	//Select setup
	fd_set readfds;
	srand(time(NULL));
	
	if (argc != 5)
	{
		perror ("Incorrect number of arguments\n");
		return 1;
	}

	// configure address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (atoi (argv[1]));
	inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
	memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof serverAddr;

	//Create UDP socket
	sock = socket (PF_INET, SOCK_DGRAM, 0);
	fcntl(sock, F_SETFL, O_NONBLOCK);

	//Send destination file name and open source file
	sendto(sock, argv[4],strlen(argv[4])+1, 0, (struct sockaddr *)&serverAddr, addr_size);
	src=fopen(argv[3], "rb");

	//Send source file
	while ((nBytes=fread(p.data,1,10,src)))
	{
		int f=-1;
		p.head.len=nBytes;
		p.head.seq_ack=state;
		p.head.cksum=0;
						
		while(f==-1)
		{
			//Random to choose correct checksum or 0
			chk=rand() % 10;
			if(chk<=1)
			{
				p.head.cksum=0;
			}
			if(chk>1)
			{
				p.head.cksum=csum((char*)&p, sizeof(struct HEADER) + sizeof(p.data));
			}

			// send packet
			if(chk>1)
			{
				sendto (sock, &p, sizeof(struct PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);
				printf("Packet sent\n");
			}

			//Set timer
			FD_ZERO(&readfds);
			FD_SET(sock, &readfds);
			tv.tv_sec=10;
			tv.tv_usec=0;

			//Use select as timer
			rv=select(sock+1, &readfds, NULL, NULL, &tv);
			
			if(rv==0)
			{
				perror("No data received, timeout\n");
			}

			//There is data to be received
			else if(rv==1)
			{
				// receive ack
				int c = recvfrom(sock, &ack, sizeof(struct HEADER), 0, NULL, NULL);
				if(ack.seq_ack==state)
				{
					printf("Correct ACK received\n");
					f=1;
					state = !state;
				}
			}
		}
	}

	//Make final packet
	p.head.len=0;
	p.head.cksum=0;
	p.head.seq_ack=state;
	p.data[0]='\0';
	p.head.cksum=csum((char*)&p, sizeof(struct HEADER) + sizeof(p.data));

	for(i=0;i<3;i++)
	{
		printf("Sending final packet %d/3 times\n", i+1);
		if(chk>1)
		{
			sendto (sock, &p, sizeof(struct HEADER) + 1, 0, (struct sockaddr *)&serverAddr, addr_size);
		}
		
		//Set timer
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);

		tv.tv_sec=10;
		tv.tv_usec=0;

		rv=select(sock+1, &readfds, NULL, NULL,  &tv);
		if(rv==1)
		{
			printf("Final ACK received\n");
			int c = recvfrom(sock, &ack, sizeof(struct HEADER), 0, NULL, NULL);
			break;
		}
		else
		{
			printf("No data received, timeout\n");
		}
	}

	//Close file and connection
	fclose(src);
	close(sock);

	return 0;
}
