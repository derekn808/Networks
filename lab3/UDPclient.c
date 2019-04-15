// UDP client
// sends a file 10 bytes at a time as a struct packet
// packet contains a header struct which holds packet length, checksum,
// and sequence or ack number
// Client waits for server to send back a correct ack to ensure that
// packet sent was correct
// Client will only send a new packet once it gets the correct ack
// Client will send a packet with 0 len to server to indicate that the
// file is done sending.
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <arpa/inet.h>

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
	int sock, portNum, nBytes;
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	struct PACKET p;
	FILE *src;
	bool state=true;
	struct HEADER ack;

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

	sendto(sock, argv[4],strlen(argv[4])+1, 0, (struct sockaddr *)&serverAddr, addr_size);
	src=fopen(argv[3], "rb");

	while ((nBytes=fread(p.data,1,10,src)))
	{
		int f=-1;
		p.head.len=nBytes;
		p.head.seq_ack=state;
		p.head.cksum=0;
				
		while(f==-1)
		{
			p.head.cksum=csum((char*)&p, sizeof(struct HEADER) + nBytes);

			// send packet
			sendto (sock, &p, sizeof(struct PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);

			// receive ack
			int c = recvfrom(sock, &ack, sizeof(struct HEADER), 0, NULL, NULL);
			if(ack.seq_ack==1)
			{
				printf("Correct ACK received\n");
				f=1;
				state = !state;
			}
		}
	}
	//send final packet
	p.head.len=0;
	p.head.cksum=0;
	p.head.seq_ack=state;
	p.data[0]='\0';
	printf("Sending final packet\n");
	sendto (sock, &p, sizeof(struct HEADER) + 1, 0, (struct sockaddr *)&serverAddr, addr_size);
	fclose(src);
	close(sock);

	return 0;
}
