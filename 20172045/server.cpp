#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <pthread.h>
#include <netdb.h>
#include <mutex> 
#include <unistd.h>
#include "serverActionHandling.h"

// to be received from command line arguments
int PORT;
char mappingFileName[128];
char rootDirectory[128];
char serverIpAddress[128];

// number of server threads, can be decided based on underlying machine and traffic
int numberOfServerThreads=1000;

// lock to have serialized access to common resouce of number of available threads
std::mutex lockOnNumberOfThreads;	


int main(int argc, char *argv[])
{

	if(argc != 6) // script name, server ip, server port, repo file, client file, root dir
	{
		printf("insufficient number of arguments\n");
		return 0;
	}
	// initializing from command line arguments
	PORT=atoi(argv[2]);
	strcpy(mappingFileName, argv[3]);
	strcpy(serverIpAddress, argv[1]);
	strcpy(rootDirectory, argv[5]);
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	pthread_t threads[numberOfServerThreads];
	pthread_t udpThreadForHeartBeat;
	pthread_t clearSilentClientThread;

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) // localhost for TCP/IP
	{
		printf("could not start server");
		exit(0);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
				&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	// Forcefully attaching socket to the port 8080
	if(bind(server_fd, (struct sockaddr *)&address, 
				sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 5) < 0) // maintaining queue for 5 connections
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	//initialize server mapping file
	FILE *mappingFile;
	// building effective file path for mapping file
	char filePath[256];
	
	strcpy(filePath, rootDirectory);
	strcat(filePath, mappingFileName);
	// reading server mapping file
	mappingFile = fopen(filePath,"r");
	if(mappingFile == NULL)
	{
		printf("could not read mapping file \n");
		return -1;
	}
	FileLocationMapping *fileLocationMapping = readServerMappingFile(mappingFile);
	fclose(mappingFile);

//	std::cout<< (fileLocationMapping->fileNameToClientIp_ClientLocation)["try.txt"][0];
	HeartBeatServerRequest *heartBeatServerRequest;
	heartBeatServerRequest = (HeartBeatServerRequest *)malloc(sizeof(HeartBeatServerRequest));
	strcpy(heartBeatServerRequest->serverIpAddress, serverIpAddress);
	heartBeatServerRequest->serverUdpPortNumber = PORT;
	// creating first thread to listen to client heart beat requests from UDP connection
	pthread_create(&udpThreadForHeartBeat, NULL, &handleIncomingHeartBeat, (void *)heartBeatServerRequest);	
	// creating remove silent clients service
	pthread_create(&clearSilentClientThread, NULL, &clearSilentClients, (void *)NULL);	
	int waiting = 0;

	while(1)
	{
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}
		
		char clientIpAddress[128];
		char buffer[1024]={0};
		inet_ntop(AF_INET, &(address.sin_addr), clientIpAddress, addrlen);

		lockOnNumberOfThreads.lock();
		if(numberOfServerThreads == 0)
		{
			lockOnNumberOfThreads.unlock();
			printf("all threads are working, putting this in waiting queue \n");
			waiting = 1;
			while(1)
			{
				sleep(2);
				lockOnNumberOfThreads.lock();

				if(numberOfServerThreads > 0)
				{
					// spwan here
					waiting = 0;
					lockOnNumberOfThreads.unlock();
					break;
				}

				lockOnNumberOfThreads.unlock();
			}	
		}
		lockOnNumberOfThreads.unlock();
		lockOnNumberOfThreads.lock();
		numberOfServerThreads--;
		valread = read( new_socket , buffer, 1024);
		ClientRequest *clientRequest;
		clientRequest = (ClientRequest *)malloc(sizeof(ClientRequest));
		strcpy(clientRequest->request, buffer);
		clientRequest->new_socket = new_socket;
		clientRequest->fileLocationMapping = fileLocationMapping;
		strcpy(clientRequest->mappingFileName, filePath);
		void *req = (void *)clientRequest;
		pthread_create(&threads[numberOfServerThreads], NULL, &handleIncomingRequestUsingThread, req);
		lockOnNumberOfThreads.unlock();
		listen(server_fd, 5);
	}
	pthread_exit(NULL);
	return 0;
}
