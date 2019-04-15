/*
 * Derek Nakamura
 * R 9:15
 * 4 arguments: port no., ip address, src, desti
 * Description: Client connects to a server, sends a destination file name,
 * opens source file, then sends the source file 10 bytes at a time.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]){
	char buffer[10];
	int sockfd, c;
	FILE *src;
	struct sockaddr_in serv_addr;

	if(argc!=5){
		printf("Not enough arguments.\n");
		return 1;
	}

	/* Open Socket
	 * domain: AF_INET - IPv4 internet protocols
	 * type: SOCK_STREAM - two-way, connection-based byte streams
	 * protocol:0
	 * return value: -1 is error
	 */
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
		printf("Socket could not be created.\n");
		return 1;
	}
	
	/* Set address
	 * htons - converts port no. short from host byte order to
	 * network byte order.
	 * inet_pton - convert address from text to binary
	 */
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(atoi(argv[1]));
	if(inet_pton(AF_INET, argv[2], &serv_addr.sin_addr)<=0){
		printf("inet_pton error.\n");
		return 1;
	}
	
	// Connect
	if(connect(sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr))>0){
		printf("Connection failed.\n");
		return 1;
	}

	// Send name of destination file
	write(sockfd, argv[4], strlen(argv[4])+1);
	
	// Open source, Send file contents, Close file
	src=fopen(argv[3], "rb");

	while(c=fread(buffer, 1, 10, src)){
		write(sockfd, buffer, c);
	}
	fclose(src);

	// Close socket
	close(sockfd);
	
	return 0;
}
