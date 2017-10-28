#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <pthread.h>
#include <mutex>
#include <unistd.h>
#include "clientActionHandling.h"


int CRS_PORT;
int FILE_DOWNLOAD_PORT;
char serverIpAddress[128];
char clientIpAddress[128];
char clientHostName[128];
char clientRootAddress[512];

// structure required to pass to outgoing crs request thread
OutgoingCRSRequest outgoingCRSRequest;
// structure required to pass to incoming file download requets thread
IncomingFileDownloadRequest incomingFileDownloadRequest;
HeartBeat heartBeat;

int main(int argc, char const *argv[])
{

	if(argc != 8)
	{
		printf("insufficient number of arguments \n");
		return 0;
	}

	// assuming all arguments are correct
	strcpy(clientHostName, argv[1]);
	strcpy(clientIpAddress, argv[2]);
	FILE_DOWNLOAD_PORT = atoi(argv[3]);
	strcpy(serverIpAddress, argv[4]);
	CRS_PORT = atoi(argv[5]);
	strcpy(clientRootAddress, argv[7]);

	struct sockaddr_in address;
	int sock = 0, valread;
//	char buffer[1024] = {0};
	
	pthread_t threads[3];
	
	//buliding outgoing request
	strcpy(outgoingCRSRequest.serverIpAddress, serverIpAddress);
	outgoingCRSRequest.serverPortNumber = CRS_PORT;
	void *outgoingRequest = (void *)&outgoingCRSRequest;
	
	// bulding incoming file download server request
	incomingFileDownloadRequest.fileDownloadPortNumber = FILE_DOWNLOAD_PORT;
	void *incomingRequest = (void *)&incomingFileDownloadRequest;

	//thread[0] will point to outgoing CRS request threads
	//thread[1] will point to incoming file download request
	//thread[2] will point to continuous outgoing heart beat requests	
	strcpy(heartBeat.serverIpAddress, serverIpAddress);
	strcpy(heartBeat.clientHostName, clientHostName);
	heartBeat.serverPortNumber = CRS_PORT;

	heartBeat.clientFileDownloadPortNumber = FILE_DOWNLOAD_PORT;
	void *heartBeatRequest = (void *)&heartBeatRequest;

	// creating outgoing thread
	pthread_create(&threads[0], NULL, &handleOutgoingCRSRequests, outgoingRequest);
	
	// creating incoming thread
	pthread_create(&threads[1], NULL, &handleIncomingFileDownloadRequest, incomingRequest);
	
	// creating sending heart beat thread thread
	pthread_create(&threads[2], NULL, &handleHeartBeats, heartBeatRequest);

	// waiting for outgoing CRS request thread
	void *status;
	pthread_join(threads[0], &status);

	return 0;
}
