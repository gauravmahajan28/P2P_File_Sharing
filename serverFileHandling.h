#include<bits/stdc++.h>
#include<unordered_map>
#include<vector>
#include<string>


enum ActionType
{
	ADD,
	REMOVE
};

enum ClientStatus
{
	ONLINE,
	OFFLINE
};



typedef struct FileLocationMapping
{
	std::unordered_map<std::string, std::vector< std::string > > fileNameToClientIp_ClientLocation; // map containing mapping from fileName to client IP address and location
	std::unordered_map<std::string, std::vector< std::string > > clientIpAddressToFileNames;
}FileLocationMapping;



// read mapping file
FileLocationMapping *readServerMappingFile(FILE *fp);


// log all incoming client requests
void logClientRequest(struct tm *now, char *clientIp, char *clientHostName, char *action, char *fileName, char *fileLocation, FILE *fp);

// update file location mapping
void updateClientFileMapping(FileLocationMapping *fileLocationMapping, char *mappingFileName);

//update currently online clients
void updateAvailableClientList(char *clientIp, char *clientHostName, ClientStatus clientStatus);






