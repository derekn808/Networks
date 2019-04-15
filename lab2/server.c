/*
 * Derek Nakamura
 * R 9:15
 * 1 argument: port number
 * Description: Creates a server that will accept a connection from a
 * client. Takes the name of a destination file, creates the file, then
 * takes input from client to write the file 5 bytes at a time.
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
	char buffer[5], dest_name[50];
	char *p;
	int connfd, listenfd, n, f=-1;
	FILE *dest;
	struct sockaddr_in serv_addr;

	if(argc!=2){
		printf("Incorrect number of arguments.\n");
		return 1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);	//host long
	serv_addr.sin_port = htons(atoi(argv[1])); 			//host short

	// create socket, bind, and listen
	listenfd = socket (AF_INET, SOCK_STREAM, 0);	//Create a socket get a tcp/ip socket
	bind (listenfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr)); //Bind the socket to an address
	listen (listenfd, 10); //Listen for connections
	
	//Accept connection, read file name, then open file for writing
	connfd=accept(listenfd, (struct sockaddr*)NULL, NULL);
	if((n=read(connfd,dest_name, sizeof(dest_name)))>0){
		dest=fopen(dest_name, "wb");
		f=1;
	}
	
	if(f==-1){
		printf("Destination file failed to open.\n");
	}
	
	//Read information, then write file, close file, close connection
	if(f==1){
		while((n=read(connfd, buffer, sizeof(buffer)))>0){
			fwrite(buffer, n, 1, dest);
		}
		fclose(dest);
		close(connfd);
		return 0;
	}
}
