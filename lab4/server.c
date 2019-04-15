// Derek Nakamura
// Lab 4
// UDP server
// Creates a new file based on name sent by client
// Server calculates a checksum on the packet sent to ensure that the
// contents of the file are not corrupted.
// Server will send an ack back to the client if the sequence number and
// the checksum match what the client sent
// Server will close the file and the connection once the client sends
// a packet of length 0 indicating the file is done sending.
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

int csum(char *buff, int buffsize)
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
	int sock, nBytes, chk, f=-1;
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	char buff[10], dest_name[40];
	struct PACKET p;
	FILE *dest;
	int state=0;
	struct HEADER ack;
	srand(time(NULL));

	if(argc!=2)
	{
		perror("Incorrect number of arguments\n");
		return 1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons((short)atoi(argv[1]));
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset((char*)serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
	addr_size = sizeof(serverStorage);

	//Create socket
	if((sock=socket(AF_INET, SOCK_DGRAM, 0)) <0)
	{
		perror("Socket error\n");
		return 1;
	}

	//Bind
	if(bind(sock,(struct sockaddr *)&serverAddr, sizeof(serverAddr))!=0)
	{
		perror("Bind error\n");
		return 1;
	}
	
	//Receive file name and open file
	if((nBytes=recvfrom(sock,dest_name, 40, 0 ,NULL, NULL))>0)
	{
		dest=fopen(dest_name, "wb");
		f=1;
	}
	if(f==-1)
	{
		perror("Destination file name not received to open.\n");
		return 1;
	}
	
	ack.len=0;
	ack.cksum=0;
	ack.seq_ack=state;
	f=-1;

	while(f==-1)
	{
		chk=rand() % 10;
		nBytes=recvfrom(sock,&p,sizeof(struct PACKET),0,(struct sockaddr *)&serverStorage, &addr_size);
		
		int sentChk=p.head.cksum;
		p.head.cksum=0;
		int sentSeq=p.head.seq_ack;

		p.head.cksum=csum((char*)&p, sizeof(struct HEADER) + sizeof(p.data));

		if(p.head.len==0)
		{
			printf("Final packet of length 0 recieved\n");
			f=1;
		}

		ack.seq_ack=!state;
		if(p.head.cksum!=sentChk)
		{
			perror("Wrong checksum\n");
			if(chk>1)
				sendto (sock, &ack, sizeof(struct HEADER), 0, (struct sockaddr *)&serverStorage, addr_size);
		}
		else if(p.head.cksum==sentChk)
		{
			printf("Correct checksum\n");
			if(p.head.seq_ack!=state)
			{
				perror("Wrong seqnumber\n");
				if(chk>1)
					sendto (sock, &ack, sizeof(struct HEADER), 0, (struct sockaddr *)&serverStorage, addr_size);
			}
			else if(p.head.seq_ack==state)
			{
				printf("Correct seqnumber\n");
				ack.seq_ack=state;
				fwrite(p.data, 1, p.head.len, dest);
				state = !state;
				if(chk>1)
				{
					sendto (sock, &ack, sizeof(struct HEADER), 0, (struct sockaddr *)&serverStorage, addr_size);
					printf("ACK sent\n");
				}
			}
		}
	}

	//Close file and connection
	fclose(dest);
	close(sock);
	return 0;

}
