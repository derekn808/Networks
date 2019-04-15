/*
 * Derek Nakamura
 * Implementation or Djikstra's/link-state algorithm
 * Takes in port no and 2 files as arguments
 * first file contains list of hosts, their ip, and their port
 * second file gives information about the cost for travelling between nodes
 * Uses UDP to connect to other nodes in the network
 * uses 3 threads:
 * 1. main - reads input from user for changes in cost from current node to
 * another node and sends changes to all other nodes
 * runs every 10 seconds and stops 30 seconds after 10 changes
 * 2. receive_info - takes information about changes and adjusts their cost
 * table
 * runs forever
 * 3. link_state - implements djikstra's to find the shortest path to other
 * connected nodes and prints out the new minimum distances
 * runs every 10-20 seconds
 *
 * num_nodes are the number of nodes in the network
 * infinite distance is 10000
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

struct MACHINES{
	char name[50];
	char ip[50];
	int port;
};

//Global Variables
int num_nodes=3;
int port2;

struct MACHINES hosts[3];
int costs[3][3];

pthread_mutex_t lock;

//helper function to calculate the index with the minimum distance from source node
int minDistance(int dist[], bool visited[], int src)
{
	int min =10000;
	int min_index=-1;
	int i;
	for(i=0; i<num_nodes;i++)
	{
		if(visited[i] == false && dist[i] <min && i!=src)
		{
			min=dist[i];
			min_index=i;
		}
	}
	return min_index;
}

//Djikstra's algorithm
void djikstra(int src)
{
	int dist[num_nodes];		//array of distances from source to index
	bool visited[num_nodes];	//true for visted node, false if not visited
	int i, j;

	//initialize distance array and visited array;
	for(i=0;i<num_nodes;i++)
	{
		dist[i] = costs[src][i];
		visited[i]=false;
	}
	dist[src]=0;
	visited[src]=true;

	//calculation to find shortest path and marks any nodes if visted
	for(i=0; i<num_nodes-1;i++)
	{
		int k = minDistance(dist,visited, src);
		visited[k] = true;
		for(j=0;j<num_nodes; j++)
		{
			if(!visited[j] && costs[k][j] && (dist[k]+costs[k][j])<dist[j])		
				dist[j] = dist[k] + costs[k][j];
		}
	}

	//Print out shortest path from source to node i
	for(i=0;i<num_nodes;i++)
	{
		printf("%d ", dist[i]);
	}
	printf("\n");	
}

//receives messages from other nodes and updates the neighbor cost table. When receiving a new cost c from x to neighbor y, it should update the cost in both costs: x to y and y to x.
void *receive_info(void* param)
{	
	struct sockaddr_storage serverStorage;
	struct sockaddr_in otherserverAddr;
	otherserverAddr.sin_family = AF_INET;
	otherserverAddr.sin_port = htons(port2);
	otherserverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset((char*)otherserverAddr.sin_zero, '\0', sizeof(otherserverAddr.sin_zero));
	socklen_t other_addr_size = sizeof(serverStorage);

	//Create socket
	int sock2=socket(AF_INET, SOCK_DGRAM, 0);

	//Bind
	bind(sock2,(struct sockaddr *)&otherserverAddr, sizeof(otherserverAddr));

	int l, j ,k;
	while(1)
	{
		int data[3];
		recvfrom(sock2, data, sizeof(data), 0, NULL, NULL);
		pthread_mutex_lock(&lock);
		for(l=0;l<num_nodes;l++)
		{
			data[l]=ntohl(data[l]);
		}
		for(l=0;l<num_nodes;l++)
		{
			if(hosts[l].port==data[0])
			{
				costs[l][data[1]]=data[2];
				costs[data[1]][l]=data[2];
			}
		}
		
		printf("Changes Received:\n");
		for(j=0; j<num_nodes; j++)
		{
			for(k=0;k<num_nodes;k++)
			{
				printf("%d\t", costs[j][k]);
			}
			printf("\n");
		}
		pthread_mutex_unlock(&lock);
	}

	return NULL;
}

//run the algorithm to update the least costs. After the algorithm executes it outputs the current least costs.
void *link_state(void* param)
{
	srand(time(NULL));
	int j, k, src;
	while(1)
	{
		int r=rand()%11 + 10;
		sleep(r);
		pthread_mutex_lock(&lock);
		for(j=0;j<num_nodes;j++)
		{
			if(hosts[j].port==port2)
			{
				src=j;
				break;
			}
		}
		printf("Node %d\t",src);
		djikstra(src);
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

//Parses file and interprets data for use. reads a new change from the keyboard every, updates the neighbor cost table,and sends messages to the other nodes using UDP
int main(int argc, char *argv[])
{
	pthread_mutex_init(&lock, NULL);

	if (argc != 4)
	{
		perror ("Incorrect number of arguments\n");
		return 1;
	}

	pthread_t thr1;
	pthread_t thr2;
	int port_no=atoi(argv[1]);
	int port1=port_no;
	FILE *host, *cost;
	int sock1;
	
	host=fopen(argv[3], "r");
	cost=fopen(argv[2], "r");
	int i, j, k;
	for(i=0;i<num_nodes;i++)
	{
		fscanf(host, "%s%s%d", hosts[i].name, hosts[i].ip, &hosts[i].port);
		fscanf(cost, "%d%d%d", &costs[i][0],&costs[i][1], &costs[i][2]);
	}
	for(i=0; i<num_nodes; i++)
	{
		for(j=0;j<num_nodes;j++)
		{
			printf("%d\t", costs[i][j]);
		}
		printf("\n");
	}
	for(i=0;i<num_nodes;i++)
	{
		printf("%s\t", hosts[i].name);
		printf("%s\t", hosts[i].ip);
		printf("%d\n", hosts[i].port);
	}
	fclose(host);
	fclose(cost);

	port2=port_no;

	//Receive info thread
	pthread_create(&thr1, NULL, receive_info, NULL);

	//Link state thread
	pthread_create(&thr2, NULL, link_state, NULL);

	// configure address
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));
	socklen_t addr_size = sizeof serverAddr;
	sock1 = socket (PF_INET, SOCK_DGRAM, 0);

	int id, new_cost, index, l;

	for(l=0;l<num_nodes;l++)
	{
		if(hosts[l].port==port_no)
		{
			index=l;
			break;
		}
	}

	i=0;
	while(1)
	{
		sleep(10);
		int packet[3];
		printf("Input id and new cost\n");
		scanf("%d %d", &id, &new_cost);
		pthread_mutex_lock(&lock);
		costs[index][id]=new_cost;
		costs[id][index]=new_cost;
		for(j=0;j<num_nodes;j++)
		{
			for(k=0;k<num_nodes;k++)
			{
				printf("%d\t", costs[j][k]);
			}
			printf("\n");
		}
		pthread_mutex_unlock(&lock);
		packet[0]=htonl(port1);
		packet[1]=htonl(id);
		packet[2]=htonl(new_cost);

		for(j=0;j<num_nodes;j++)
		{
			if(j==index)
			{
				continue;
			}
			else
			{
				serverAddr.sin_port = htons (hosts[j].port);
				inet_pton (AF_INET, hosts[j].ip, &serverAddr.sin_addr.s_addr);
				sendto(sock1, packet, sizeof(packet),0, (struct sockaddr *)&serverAddr, addr_size);
			}
		}
		i++;

		if(i==9)
		{
			sleep(30);
			break;
		}
	}

	return 0;
}
