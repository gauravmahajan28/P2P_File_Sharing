
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


typedef struct FileLocationAtClient
{
	char *fileName;
	char *clientHostName;
	char *clientIp;
}FileLocationAtClient;


typedef struct OutgoingCRSRequest
{
	char serverIpAddress[128];
	int serverPortNumber;
}OutgoingCRSRequest;


typedef struct IncomingFileDownloadRequest
{
	int fileDownloadPortNumber;
}IncomingFileDownloadRequest;


typedef struct HeartBeat
{
	char serverIpAddress[128];
	char clientHostName[128];
	int serverPortNumber;
	int clientFileDownloadPortNumber;
}HeartBeat;


typedef struct FileUploadRequest
{
	char request[128];
	int socket;
}FileUploadRequest;


typedef struct RPCRequest
{
	char command[128];
	char arguments[128][128];
}RPCRequest;

typedef struct RPCResponse
{
	char response[4096];
}RPCResponse;



// returns the location of file descriptor
FileLocationAtClient *findFileLocationAtClient(char *fileDescriptor);

// share the location of file
void shareFileLocationAtClient(char *fileName, char *fileLocation, char *clientHostName, char *clientIp);

// deregister file location
void removeFileLocationAtClient(char *fileName, char *fileLocation, char *clientHostName, char *clientIp);

// execute remote RPC command
void executeRemoteProceduralCallAtClient(char *command, char **arguments);

// create thread to rquest to CRS
void *handleOutgoingCRSRequests(void *req);

// create thread to handle incoming file download request
void *handleIncomingFileDownloadRequest(void *req);

// create thread to handle incoming file download request
void *handleHeartBeats(void *req);

// log all client side activities
void logClientRequest(struct tm *now, char *clientIp, char *clientHostName, char *action, char *fileName, char *fileLocation,  FILE *fp);


