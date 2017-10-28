#include<iostream>
#include "serverActionHandling.h"
#include<mutex>
#include<string>
#include<vector>
#include<sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>


extern int numberOfServerThreads;
extern std::mutex lockOnNumberOfThreads;


extern int PORT;
extern char serverIpAddress[128];

std::mutex lockOnLoggingFile;
std::mutex lockOnMappingFile;


std::unordered_map<std::string, time_t> onlineClientsWithTimeout;
std::mutex lockOnOnlineClients;

std::unordered_map<std::string, int> clientsWithPort;

std::unordered_map<std::string, std::string> clientsWithIpAddress;


// find location of file in mapping file
void findFileLocation(char *fileName, FileLocationMapping *fileLocationMapping, int new_socket)
{
	std::string key(fileName);
	std::vector<std::string> responseVector;
	char response[4096];
	strcpy(response, "");
	
	lockOnMappingFile.lock();

	// search for substring
	if((fileLocationMapping->fileNameToClientIp_ClientLocation).find(key) == (fileLocationMapping->fileNameToClientIp_ClientLocation).end())
	{
		std::unordered_map<std::string, std::vector< std::string> >::iterator mapIterator;

		for ( mapIterator = (fileLocationMapping->fileNameToClientIp_ClientLocation).begin(); mapIterator !=  (fileLocationMapping->fileNameToClientIp_ClientLocation).end(); ++mapIterator )
		{
		
			std::string search = mapIterator->first;
			std::string toBeSearched(fileName);
			std::transform(search.begin(), search.end(), search.begin(), ::tolower);
			std::transform(toBeSearched.begin(), toBeSearched.end(), toBeSearched.begin(), ::tolower);
			std::size_t found = (search).find(toBeSearched, 0);
			// string is found
			if(found != std::string::npos)
			{
				for(unsigned int counter = 0; counter < (mapIterator->second).size(); counter++)
				{
					char clientIpAddress[128], clientFileLocation[128], clientHostName[128];
					strcpy(clientIpAddress, "");
					strcpy(clientFileLocation, "");
					strcpy(clientHostName, "");
					char probableResponse[4096];
					strcpy(probableResponse, ((mapIterator->second)[counter]).c_str());

					char *token1, *token2;
					token1 = strtok(probableResponse, "#");
					if(token1 != NULL)
					{
						strcpy(clientFileLocation, token1);
						token2 = strtok(NULL, "#");
						if(token2 != NULL)
						{
							strcpy(clientHostName, token2);
						}

					}

					if(clientsWithPort.find(std::string(clientHostName)) == clientsWithPort.end())
					{
						(mapIterator->second).erase((mapIterator->second).begin() + counter);

					}
					else
					{
						strcat(response, "&");
						strcat(response, (mapIterator->first).c_str());
						strcat(response, "#");
						strcat(response, clientHostName);
						strcat(response, "#");
						strcat(response, clientFileLocation);
						strcat(response, "#");
						strcat(response, (clientsWithIpAddress[std::string(clientHostName)]).c_str());
						strcat(response, "#");
						strcat(response, (std::to_string(clientsWithPort[std::string(clientHostName)])).c_str());
					}	
				}
			}	
		}

		if(strlen(response) == 0)
			strcpy(response, "FILE NOT FOUND");

	}
	else
	{
		
		responseVector = (fileLocationMapping->fileNameToClientIp_ClientLocation)[key];
		strcpy(response, "");
		for(unsigned int counter = 0; counter < responseVector.size(); counter++)
		{
			char clientIpAddress[128], clientFileLocation[128], clientHostName[128];
			strcpy(clientIpAddress, "");
			strcpy(clientFileLocation, "");
			strcpy(clientHostName, "");
			char probableResponse[4096];
			strcpy(probableResponse, (responseVector[counter]).c_str());

			char *token1, *token2;
			token1 = strtok(probableResponse, "#");
			if(token1 != NULL)
			{
				strcpy(clientFileLocation, token1);
				token2 = strtok(NULL, "#");
				if(token2 != NULL)
				{
					strcpy(clientHostName, token2);
				}

			}

			std::unordered_map<std::string, int >::iterator it;
			if(clientsWithPort.find(std::string(clientHostName)) == clientsWithPort.end())
			{
				
				(responseVector).erase((responseVector).begin() + counter);

			}
			else
			{
				strcat(response, "&");
				strcat(response, fileName);
				strcat(response, "#");
				strcat(response, clientHostName);
				strcat(response, "#");
				strcat(response, clientFileLocation);
				strcat(response, "#");
				strcat(response, (clientsWithIpAddress[std::string(clientHostName)]).c_str());
				strcat(response, "#");
				strcat(response, (std::to_string(clientsWithPort[std::string(clientHostName)])).c_str());
			}	
		}
	}
	lockOnMappingFile.unlock();
	send(new_socket, response, strlen(response), 0);
}


// add file name to mapping file / update it
void shareFileLocation(char *fileName, char *fileLocation, char *clientHostName, char *clientIpAddress, char *mappingFileName, FileLocationMapping *fileLocationMapping, int new_socket)
{
	lockOnMappingFile.lock();
	// update in file
	FILE *mappingFile = fopen(mappingFileName, "a");
	fprintf(mappingFile, "%s:%s:%s\n",fileName, fileLocation, clientHostName);
	fclose(mappingFile);

	char entriesToBeAdded[512];
	std::vector<std::string> fileLocationVector;
	strcpy(entriesToBeAdded, "");
	strcat(entriesToBeAdded, fileLocation);
	strcat(entriesToBeAdded, "#");
	strcat(entriesToBeAdded, clientHostName);

	std::string key(fileName), value(entriesToBeAdded);
	// file not found 
	if((fileLocationMapping->fileNameToClientIp_ClientLocation).find(key) == (fileLocationMapping->fileNameToClientIp_ClientLocation).end())
	{
		fileLocationVector.push_back(value);
		(fileLocationMapping->fileNameToClientIp_ClientLocation)[key] = fileLocationVector;
	}
	// just add entry to vector
	else
	{
		fileLocationVector = (fileLocationMapping->fileNameToClientIp_ClientLocation)[key];
		fileLocationVector.push_back(value);
		(fileLocationMapping->fileNameToClientIp_ClientLocation)[key] = fileLocationVector;
	}
	send(new_socket, "FILE SHARED", strlen("FILE SHARED"), 0);
	lockOnMappingFile.unlock();

}


// remove file name to mapping file / update it
void removeFileLocation(char *fileName, char *fileLocation, char *clientHostName, char *clientIpAddress, char *mappingFileName, FileLocationMapping *fileLocationMapping, int new_socket)
{

	lockOnMappingFile.lock();
	// update in file
	/*	FILE *mappingFile = fopen(mappingFileName, "a");
		fprintf("\n %s %s %s %s",clientIpAddress, fileName, fileLocation, clientHostName);
		fclose(mappingFile);
	 */	
	char entriesToBeRemoved[512];
	std::vector<std::string> fileLocationVector;
	strcpy(entriesToBeRemoved, "");
	strcat(entriesToBeRemoved, fileLocation);
	strcat(entriesToBeRemoved, "#");
	strcat(entriesToBeRemoved, clientHostName);
	std::string key(fileName), value(entriesToBeRemoved);



	if((fileLocationMapping->fileNameToClientIp_ClientLocation).find(key) == (fileLocationMapping->fileNameToClientIp_ClientLocation).end())
	{
		// don't do anything
	}
	// just add entry to vector
	else
	{
		int changed = 0;
		fileLocationVector = (fileLocationMapping->fileNameToClientIp_ClientLocation)[key];

		for(unsigned int counter = 0; counter < fileLocationVector.size(); counter++)
		{
			char entry[512], ip[128], host[128], location[128];
			strcpy(host, "");
			strcpy(location, "");
			strcpy(entry, fileLocationVector[counter].c_str());

			char *token1, *token2, *token3;

			token1 = strtok(entry, "#");

			if(token1 != NULL)
			{
				strcpy(location, token1);
				token2 = strtok(NULL, "#");

				if(token2 != NULL)
				{
					strcpy(host, token2);
				}

			}
			if((strcmp(host, clientHostName)==0) && (strcmp(location, fileLocation)==0))
			{
				changed = 1;
				fileLocationVector.erase(counter + fileLocationVector.begin());
				if(fileLocationVector.size() == 0)
				{	
					(fileLocationMapping->fileNameToClientIp_ClientLocation).erase(key);
					break;
				}
				else
				{
					(fileLocationMapping->fileNameToClientIp_ClientLocation)[key] = fileLocationVector;

				}
			}	

		}// for
		if(changed == 1)
		{
			updateClientFileMapping(fileLocationMapping, mappingFileName);
		}		

	}// else
	send(new_socket, "FILE_REMOVED", strlen("FILE_REMOVED"), 0);
	lockOnMappingFile.unlock();
}


// entry point for client request
void *handleIncomingRequestUsingThread(void *req)
{
	ClientRequest *clientRequest = (ClientRequest *)req;
	char request[512];
	char clientIpAddress[128];
	char clientHostName[128];
	char mappingFileName[128];

	int new_socket;
	FileLocationMapping *fileLocationMapping;

	strcpy(request, clientRequest->request);

//	printf("request is received to server as %s\n",request);

	// request will be of the form hostName#clientIpAddress#action#fileName#fileLocation
	// parse incoming request
	char *token1 = strtok(request, "#");
	char *token2, *token3, *token4, *token5;
	char action[128], fileName[128], fileLocation[128];
	strcpy(action , "");
	strcpy(fileName , "");
	strcpy(fileLocation , "");
	while(token1 != NULL)
	{
		strcpy(clientHostName, token1);
		token2= strtok(NULL, "#");
		if(token2 == NULL)
			break;
		strcpy(clientIpAddress, token2);	
		token3= strtok(NULL, "#");
		if(token3 == NULL)
			break;
		strcpy(action, token3);	

		token4= strtok(NULL, "#");
		if(token4 == NULL)
			break;
		strcpy(fileName, token4);	

		token5= strtok(NULL, "#");
		if(token5 == NULL)
			break;
		strcpy(fileLocation, token5);	

	}


	new_socket = clientRequest->new_socket;
	fileLocationMapping = clientRequest->fileLocationMapping;
	strcpy(mappingFileName, clientRequest->mappingFileName);

	// LOGGING REQUEST
	lockOnLoggingFile.lock();
	time_t currentTime = time(0);
	struct tm *now = localtime(&currentTime);
	char date[128];
	strcpy(date, "");
	strcat(date, (std::to_string(now->tm_year + 1900)).c_str());
	strcat(date, (std::to_string(now->tm_mon + 1)).c_str());
	strcat(date, (std::to_string(now->tm_mday + 1)).c_str());
	strcat(date, "@server.log");
	FILE *loggingFile = fopen(date, "a");
	logClientRequest(now, clientIpAddress, clientHostName, action, fileName, fileLocation, loggingFile);
	fclose(loggingFile);
	lockOnLoggingFile.unlock();
	//LOGGING REQUEST

	if(strcmp(action, SEARCH_REQUEST) == 0)
	{
		findFileLocation(fileName, fileLocationMapping, new_socket);
	}
	else if(strcmp(action, SHARE_REQUEST) == 0)
	{
		shareFileLocation(fileName, fileLocation, clientHostName, clientIpAddress, mappingFileName, fileLocationMapping, new_socket);

	}
	else if(strcmp(action, UNSHARE_REQUEST) == 0)
	{
		removeFileLocation(fileName, fileLocation, clientHostName, clientIpAddress, mappingFileName, fileLocationMapping, new_socket);
	}
	else
		send(new_socket, "NOT FOUND", strlen("NOT FOUND"), 0);

	lockOnNumberOfThreads.lock();	
	numberOfServerThreads++;
	lockOnNumberOfThreads.unlock();
	close(new_socket);
	return NULL;
}

void *handleIncomingHeartBeat(void *req)
{
//	printf("staring udp server\n");
	HeartBeatServerRequest *heartBeatServerRequest = (HeartBeatServerRequest *)req;

	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	socklen_t addrlen = sizeof(address);     


	if ((server_fd = socket(AF_INET, SOCK_DGRAM,0)) == 0) // localhost for TCP/IP
	{
		printf("could not start server");
		exit(0);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr =  htonl(INADDR_ANY);
	address.sin_port = htons( PORT );


	if(bind(server_fd, (struct sockaddr *)&address,
				sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
//	printf("started heart beat server successfully \n");
	while(1)
	{
		char buf[1024]={0};

//		printf("waiting on port %d\n", PORT);
		recvfrom(server_fd, buf, 1-24, 0, (struct sockaddr *)&address, &addrlen);

		char *token1, *token2, *token3, *token4;
		char hostName[128], portN[128], clientIpAddress[128];
		strcpy(hostName, "");
		strcpy(clientIpAddress, "");
		strcpy(portN, "");
		int portNumber;

		// format of request is hello#hostName#port#clientIpAddress

		token1 = strtok(buf, "#");
		if(token1 != NULL)
		{
			token2 = strtok(NULL, "#");

			if(token2 != NULL)
			{		
				strcpy(hostName, token2);

				if(token2 != NULL)
				{
					token3 = strtok(NULL, "#");

					if(token3 != NULL)
					{
						strcpy(portN, token3);
						portNumber = atoi(portN);

						token4 = strtok(NULL, "#");

						if(token4 != NULL)
						{
							strcpy(clientIpAddress, token4);
						}

					}

				}

			}
		}
		
		lockOnOnlineClients.lock();
		onlineClientsWithTimeout[std::string(hostName)] = time(0) + 60; // one minute
		clientsWithPort[std::string(hostName)] = portNumber;
		clientsWithIpAddress[std::string(hostName)] = std::string(clientIpAddress);
		std::unordered_map<std::string, int >::iterator it;

		if(clientsWithPort.empty())
		{
			printf("map is empty\n");
		}
		else
		{
			for (  it  = clientsWithPort.begin(); it != clientsWithPort.end(); ++it )
			{
				std::cout << " " << it->first << ":\n";
				std::cout<< " - " << it->second << " - ";
				printf("\n");
			}
			std::cout << std::endl;
		}
		lockOnOnlineClients.unlock();
	}
	close(new_socket);
}


// clear silent client entries
void *clearSilentClients(void *req)
{
	while(1)
	{
		lockOnOnlineClients.lock();
		std::unordered_map<std::string, time_t>::iterator mapIterator;
		for ( mapIterator =onlineClientsWithTimeout.begin(); mapIterator !=  onlineClientsWithTimeout.end(); ++mapIterator )
		{
			std::string hostName = mapIterator->first;
			time_t timeOut = mapIterator->second;
			time_t currentTime = time(0);
			if(timeOut < currentTime)
			{
				printf("\n %s cleared",hostName.c_str());
				onlineClientsWithTimeout.erase(hostName);
				clientsWithPort.erase(hostName);
				clientsWithIpAddress.erase(hostName);
			}

		}
		lockOnOnlineClients.unlock();
		sleep(120); // 3 minutes
	}
	return NULL;

}	
