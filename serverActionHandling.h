#include "serverFileHandling.h"

#define SEARCH_REQUEST "search"
#define SHARE_REQUEST "share"
#define UNSHARE_REQUEST "del"


typedef struct FileLocation
{
	char *fileName;
	char *clientHostName;
	char *clientIp;
}FileLocation;

typedef struct ClientRequest
{
	char request[128];
	int new_socket;
	FileLocationMapping *fileLocationMapping;
	char clientIpAddress[128];
	char clientHostName[128];
	char mappingFileName[128];
	
}ClientRequest;


typedef struct HeartBeatServerRequest
{	
	char serverIpAddress[128];
	int serverUdpPortNumber;

}HeartBeatServerRequest;


// returns the location of file descriptor
void findFileLocation(char *fileName, FileLocationMapping *fileLocationMapping, int new_socket);

// share the location of file
void shareFileLocation(char *fileName, char *fileLocation, char *clientHostName, char *clientIp, char *mappingFileName, FileLocationMapping *fileLocationMapping, int new_socket);

// deregister file location
void removeFileLocation(char *fileName, char *fileLocation, char *clientHostName, char *clientIp, char *mappingFileName, FileLocationMapping *fileLocationMapping, int new_socket);

// entry point for incoming client requests
void *handleIncomingRequestUsingThread(void *request);

// entry point for incoming heartbeats
void *handleIncomingHeartBeat(void *request);

// entry point for incoming heartbeats
void *clearSilentClients(void *request);

