#include <iostream>
#include "clientActionHandling.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <mutex>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <vector>
#include <unistd.h>


int numberOfFileDownloadThreads = 100;

std::mutex lockOnNumberOfFileDownloadThreads;
std::mutex lockOnLoggingFile;

RPCRequest rpcRequest;
RPCResponse rpcResponse;

std::mutex lockOnIndividualResults;
std::vector<std::string> individualResults;

extern int CRS_PORT;
extern int FILE_DOWNLOAD_PORT;
extern char serverIpAddress[128];
extern char clientIpAddress[128];
extern char clientHostName[128];
extern char clientRootAddress[512];

// formatting of user entered commands
char *formatCommand(char command[128])
{
	char *formattedCommand;
	formattedCommand = (char *)malloc(sizeof(char) * (strlen(command) * 2));

	char arg1[128], arg2[128], arg3[128], arg4[128];
	unsigned int counter;
	unsigned int index;
	counter = 0;
	index = 0;
	while(counter < strlen(command))
	{
		if(command[counter] == ' ')
			break;
		arg1[index++] = command[counter++];
	}
	arg1[index] = '\0';
	
	counter++;
	counter++;
	index = 0;
	while(counter < strlen(command))
	{
		if(command[counter] == '"')
			break;
		arg2[index++] = command[counter++];
	}
	arg2[index] = '\0';
	
	counter++;
	counter++;
	counter++;
	index = 0;
	while(counter < strlen(command))
	{
		if(command[counter] == '"')
			break;
		arg3[index++] = command[counter++];
	}
	arg3[index] = '\0';
	
	counter++;
	counter++;
	counter++;
	index = 0;
	while(counter < strlen(command))
	{
		if(command[counter] == '"')
			break;
		arg4[index++] = command[counter++];
	}
	arg4[index] = '\0';

	if(strcmp(arg1, "search") == 0)
	{

		char temp[128];
		int index = 0;
		for(int counter = 0; counter < (strlen(arg2)); counter++)
		{
			if(arg2[counter] != '"')
				temp[index++] = arg2[counter];
		}
		temp[index] = '\0';
		strcpy(arg2, temp);
		strcpy(formattedCommand, arg1);
		strcat(formattedCommand, "#");
		strcat(formattedCommand, arg2);
		return formattedCommand;

	}
	else if(strcmp(arg1, "share") == 0)
	{
		char temp[128];
		unsigned int index = 0, counter;
		for(counter = 0; counter < (strlen(arg2)); counter++)
		{
			if(arg2[counter] != '"')
				temp[index++] = arg2[counter];
		}
		temp[index] = '\0';

		for(counter = (strlen(temp)-1); counter >=0; counter--)
		{
			if(temp[counter] == '/')
				break;
		}
		unsigned int middleCounter = counter;
		counter++;
		index = 0;
		while(counter < strlen(temp))
		{
			arg2[index++] = temp[counter++];
		}
		arg2[index] = '\0';

		char testFileLocation[256];
		strcpy(testFileLocation, clientRootAddress);
		strcat(testFileLocation, temp);
		FILE *fp = fopen(testFileLocation, "r");
		if(fp == NULL)
		{
			return "ERROR_INVALID_FILE";
		}
		else
			fclose(fp);


		counter = 0;
		index = 0;
		while(counter <= middleCounter)
		{
			arg3[index++] = temp[counter++];

		}
		arg3[index] = '\0';
		strcpy(formattedCommand, arg1);
		strcat(formattedCommand, "#");
		strcat(formattedCommand, arg2);
		strcat(formattedCommand, "#");
//		strcat(formattedCommand, clientRootAddress);
		strcat(formattedCommand, arg3);
		return formattedCommand;

	}
	else if(strcmp(arg1,"del") == 0)
	{
		char temp[128];
		unsigned int index = 0, counter;
		for(counter = 0; counter < (strlen(arg2)); counter++)
		{
			if(arg2[counter] != '"')
				temp[index++] = arg2[counter];
		}
		temp[index] = '\0';

		for(counter = (strlen(temp)-1); counter >=0; counter--)
		{
			if(temp[counter] == '/')
				break;
		}
		unsigned int middleCounter = counter;
		counter++;
		index = 0;
		while(counter < strlen(temp))
		{
			arg2[index++] = temp[counter++];
		}
		arg2[index] = '\0';

		counter = 0;
		index = 0;
		while(counter <= middleCounter)
		{
			arg3[index++] = temp[counter++];

		}
		arg3[index] = '\0';
		strcpy(formattedCommand, arg1);
		strcat(formattedCommand, "#");
		strcat(formattedCommand, arg2);
		strcat(formattedCommand, "#");
//		strcat(formattedCommand, clientRootAddress);
		strcat(formattedCommand, arg3);
		return formattedCommand;
	}
	else if(strcmp(arg1, "exec") == 0 || strcmp(arg1, "get") == 0)
	{
		if(individualResults.size() == 0)
		{
			return "ERROR_SEARCH_FILE_FIRST";
		}
		
		char temp[128];
		unsigned int index = 0;
		for(unsigned int counter = 0; counter < (strlen(arg2)); counter++)
		{
			if(arg2[counter] != '"')
				temp[index++] = arg2[counter];
		}
		temp[index] = '\0';
		strcpy(arg2, temp);
		
		char temp2[128];
		index = 0;
		for(int counter = 0; counter < (strlen(arg3)); counter++)
		{
			if(arg3[counter] != '"')
				temp2[index++] = arg3[counter];
		}
		temp2[index] = '\0';
		strcpy(arg3, temp2);
		
		strcpy(formattedCommand, arg1);
		strcat(formattedCommand, "#");
		strcat(formattedCommand, arg2);
		strcat(formattedCommand, "#");
		strcat(formattedCommand, arg3);
		return formattedCommand;
	}
}

//log client request
void logClientRequest(struct tm *now, char *clientIpAddress,  char *clientHostName, char *action, char *fileName, char *fileLocation, FILE *fp)
{
	fprintf(fp, "%d-%d-%d %s %s %s-%s-%s\n", now->tm_hour, now->tm_min, now->tm_sec, clientHostName, clientIpAddress, action, fileName, fileLocation);

}


// execute RPC Command , store result in temp file and return result
void handleRPCRequest(int new_socket, char command[4096])
{
	FILE* file = popen(command, "r");
	char ch;
	std::string str;
	while((ch = fgetc(file))!= EOF)
		str+=ch;

	int length = str.length();
	length++;
	printf("%s\n",str.c_str());
	pclose(file);
	send(new_socket, &(length), sizeof(int), 0);
	sleep(1);
	send(new_socket, str.c_str(), str.length(), 0);

}

// for each thread for file upload
void *handleFileUploadThread(void *req)
{
	FileUploadRequest *fileUploadRequest = (FileUploadRequest *)req;
	struct stat fileSize;

	int new_socket = fileUploadRequest->socket;
	char request[128];
	strcpy(request, fileUploadRequest->request);
	
	// parse command
	char *token1, *token2, *token3, *token4, *token5, *token6;
	char action[128], fileName[128], fileLocation[128];
	char clientHostName[128], clientIpAddress[128], arguments[128];
	strcpy(action, "");
	strcpy(clientHostName, "");
	strcpy(clientIpAddress, "");
	strcpy(fileName, "");
	strcpy(fileLocation, "");
	token1 = strtok(request, "#");
	if(token1 != NULL)
	{
		strcpy(clientHostName, token1);
		token2 = strtok(NULL, "#");
		if(token2 != NULL)
		{
			strcpy(clientIpAddress, token2);
			token3 = strtok(NULL, "#");
			if(token3 != NULL)
			{
				strcpy(action, token3);
				token4 = strtok(NULL, "#");
				if(token4 != NULL)
				{
					strcpy(fileName, token4);

					token5 = strtok(NULL, "#");
					if(token5 != NULL)
					{
						strcpy(fileLocation, token5);
						token6 = strtok(NULL, "#");
						if(token6 != NULL)
						{
							strcpy(arguments, token6);
						}
					}
				}

			}	
		}
	}
	lockOnLoggingFile.lock();
	time_t currentTime = time(0);
	struct tm *now = localtime(&currentTime);
	char date[128];
	strcpy(date, "");
	strcat(date, (std::to_string(now->tm_year + 1900)).c_str());
	strcat(date, (std::to_string(now->tm_mon + 1)).c_str());
	strcat(date, (std::to_string(now->tm_mday + 1)).c_str());
	strcat(date, "@client.log");
	FILE *loggingFile = fopen(date, "a");
	logClientRequest(now, clientIpAddress,clientHostName, action, fileName, fileLocation, loggingFile);
	fclose(loggingFile);
	lockOnLoggingFile.unlock();

	if(strcmp(action, "exec") == 0)
	{
//		printf("execute command is received \n");
		handleRPCRequest(new_socket, arguments);
		return NULL;
	}

	strcat(fileLocation, fileName);
	char finalFileName[512];
	strcpy(finalFileName, clientRootAddress);
	strcat(finalFileName, fileLocation);
	
	stat(finalFileName, &fileSize);



	int size = fileSize.st_size;
	// SEND FILE SIZE FIRST
	send(new_socket, &size , sizeof(int), 0);
	
	int fd = open(finalFileName , O_RDONLY);
	sleep(2);

	while(size > 0)
	{
		/* First read file in chunks of 256 bytes */
		char buff[1024]={0};
		int numberOfBytesRead, numberOfBytesSent;

		if(size >= 1024)
		{
			numberOfBytesRead = read(fd, buff, 1024);
			numberOfBytesSent = send(new_socket, buff, numberOfBytesRead, 0);
		}
		else
		{
			numberOfBytesRead = read(fd, buff, size);
			numberOfBytesSent = send(new_socket, buff, numberOfBytesRead, 0);

		}
		size = size - numberOfBytesSent;
		printf("sent %d bytes successfully, %d bytes remaining \n", numberOfBytesSent, size);
		usleep(2000);
	}
	close(fd);
	close(new_socket);
	return NULL;

}


// act as file download thread
void *handleIncomingFileDownloadRequest(void *req)
{
	//IncomingFileDownloadRequest *incomingFileDownloadRequest = (IncomingFileDownloadRequest *)req;
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(FILE_DOWNLOAD_PORT);
	address.sin_addr.s_addr = INADDR_ANY;

	pthread_t threads[numberOfFileDownloadThreads];

	int server_fd;
	int opt = 1, new_socket;
	int addrlen = sizeof(address);

	char buffer[1024] = {0};
	int valread;

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) // localhost for TCP/IP
	{
		printf("could not start server");
		return NULL;
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
				&opt, sizeof(opt)))
	{
		perror("setsockopt");
		return NULL;
	}
	if(bind(server_fd, (struct sockaddr *)&address,
				sizeof(address))<0)
	{
		perror("bind failed");
		return NULL;
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		return NULL;
	}

	//	printf("starting file download server \n");
	while(1)
	{

		if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
						(socklen_t*)&addrlen))<0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		char clientIpAddress[128];
		inet_ntop(AF_INET, &(address.sin_addr), clientIpAddress, addrlen);

		FileUploadRequest *fileUploadRequest;
		fileUploadRequest = (FileUploadRequest *)malloc(sizeof(FileUploadRequest));

		valread = read( new_socket , buffer, 1024);
		strcpy(fileUploadRequest->request, buffer);
		fileUploadRequest->socket = new_socket;
		
		lockOnNumberOfFileDownloadThreads.lock();
		if(numberOfFileDownloadThreads== 0)
		{
			lockOnNumberOfFileDownloadThreads.unlock();
			printf("all threads are working, putting this in waiting queue \n");
			// server waits
			while(1)
			{
				sleep(2);
				lockOnNumberOfFileDownloadThreads.lock();

				if(numberOfFileDownloadThreads > 0)
				{
					// spwan here
					lockOnNumberOfFileDownloadThreads.unlock();
					break;
				}

				lockOnNumberOfFileDownloadThreads.unlock();
			}
		}
		numberOfFileDownloadThreads--;
		void *req = (void *)fileUploadRequest;
		pthread_create(&threads[numberOfFileDownloadThreads], NULL, &handleFileUploadThread, req);
		lockOnNumberOfFileDownloadThreads.unlock();
		listen(server_fd, 3);
	}// while 1
	pthread_exit(NULL);
	return 0;
}


// to handle all outgoing request to CRS server
void *handleOutgoingCRSRequests(void *req)
{
	struct sockaddr_in address;
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	char buffer[1024] = {0};
	OutgoingCRSRequest *outgoingCRSRequest = (OutgoingCRSRequest *)req;

	while(1)
	{
		fflush(stdin);
		char strIp[128];
		printf("press enter command to send to CRS  / To Another Client\n");
		gets(strIp);
		
		char command[512];
		strcpy(command, formatCommand(strIp));

		if(strcmp("ERROR_SEARCH_FILE_FIRST", command) == 0)
		{
			printf("\n please search file first \n");
			continue;
		}
		else if(strcmp("ERROR_INVALID_FILE", command) == 0)
		{
			printf("\n file does not exist \n");
			continue;
		}


		std::string str(command);
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("\n Socket creation error \n");
			return NULL;
		}
		memset(&serv_addr, '0', sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		if((str.find("get") != std::string::npos))
		{
			// incoming command is of the form 
			//get#1#opfilename
			char index[30], opFileName[512];
			int counter = 0;
			int ct = 0;

			while(command[counter] != '#')
				counter++;
			counter++;	
			while(counter < strlen(command) && command[counter] != '#')
			{
				index[ct++] = command[counter];		
				counter++;
			}
			index[ct] = '\0';
			counter++;
			ct = 0;
			while(counter < strlen(command))
			{
				opFileName[ct++] = command[counter];
				counter++;
			}

			opFileName[ct] = '\0';

			int entryInVector = atoi(index);
			char *tk1, *tk2, *tk3, *tk4, *tk5;
			char fileName[128], chn[128], cfl[128], cia[128], clientPort[128];
			char resp[128];
			
			lockOnIndividualResults.lock();
			strcpy(resp, individualResults[entryInVector-1].c_str());
			lockOnIndividualResults.unlock();
			tk1 = strtok(resp, "#");
			if(tk1 != NULL)
			{
				strcpy(fileName, tk1);
				tk2 = strtok(NULL, "#");

				if(tk2 != NULL)
				{
					strcpy(chn, tk2);	
					tk3 = strtok(NULL, "#");

					if(tk3 != NULL)
					{
						strcpy(cfl, tk3);	
						tk4 = strtok(NULL, "#");

						if(tk4 != NULL)
						{
							strcpy(cia, tk4);	
							tk5 = strtok(NULL, "#");

							if(tk5 != NULL)
							{
								strcpy(clientPort, tk5);	
							}
						}
					}
				}
			}		

			// send request to client server
			serv_addr.sin_port = htons(atoi(clientPort));
			
			if(inet_pton(AF_INET, cia , &serv_addr.sin_addr)<=0) 
			{
				printf("\nInvalid address/ Address not supported \n");
				return NULL;
			}
			if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
			{
				printf("\nConnection Failed \n");
				return NULL;
			}

			char commandToBeSent[4096];
			strcpy(commandToBeSent, "");
			strcpy(commandToBeSent, clientHostName);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, clientIpAddress);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, "get");
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, fileName);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, cfl);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, opFileName);
			
			send(sock , commandToBeSent , strlen(commandToBeSent) , 0 );
			//read file size
			int sizeOfFile;
			read( sock , &sizeOfFile, sizeof(int));
//			printf("received file  size as %d\n",sizeOfFile);
			FILE *fp;
			char finalName[256];
			strcpy(finalName, clientRootAddress);
			strcat(finalName, opFileName);
			// FILE IS CREATED AT client root address + file name
			fp = fopen(finalName, "w+"); 
			if(NULL == fp)
			{
				printf("Error opening file");
				return NULL;
			}
			int bytesReceived;
			int i = 0;
			while(sizeOfFile > 0)
			{
				char buffer[1026] = {0};
				if(sizeOfFile >= 1024)
					bytesReceived = read( sock , buffer, 1024);
				else	
					bytesReceived = read( sock , buffer, sizeOfFile);
				sizeOfFile = sizeOfFile - bytesReceived;
				write(fileno(fp), buffer, bytesReceived);
				printf("received %d bytes successfully, %d bytes remaining \n",bytesReceived, sizeOfFile);
			}
		}
		else if(str.find("exec") != std::string::npos)
		{
			
			// execute#1#ls -lr
			char index[30], commandArguments[128];
			int counter = 0;
			int ct = 0;

			while(command[counter] != '#')
				counter++;
			counter++;	
			while(counter < strlen(command) && command[counter] != '#')
			{
				index[ct++] = command[counter];		
				counter++;
			}
			index[ct] = '\0';
			counter++;
			ct = 0;
			while(counter < strlen(command))
			{
				commandArguments[ct++] = command[counter];
				counter++;
			}
			commandArguments[ct] = '\0';

			int entryInVector = atoi(index);

			char *token1, *token2, *token3, *token4, *token5;
			char fileName[128], chn[128], cfl[128], cia[128], clientPort[128];
			char resp[512];
			lockOnIndividualResults.lock();
			strcpy(resp, individualResults[entryInVector-1].c_str());
			lockOnIndividualResults.unlock();
			token1 = strtok(resp, "#");
			if(token1 != NULL)
			{
				strcpy(fileName, token1);
				token2 = strtok(NULL, "#");

				if(token2 != NULL)
				{
					strcpy(chn, token2);	
					token3 = strtok(NULL, "#");

					if(token3 != NULL)
					{
						strcpy(cfl, token3);	
						token4 = strtok(NULL, "#");

						if(token4 != NULL)
						{
							strcpy(cia, token4);	
							token5 = strtok(NULL, "#");

							if(token5 != NULL)
							{
								strcpy(clientPort, token5);	
							}
						}
					}
				}
			}


			serv_addr.sin_port = htons(atoi(clientPort));
			if(inet_pton(AF_INET, cia, &serv_addr.sin_addr)<=0) 
			{
				printf("\nInvalid address/ Address not supported \n");
				return NULL;
			}
			if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
			{
				printf("\nConnection Failed \n");
				return NULL;
			}
			//	send(sock , rpcRequest , sizeof(rpcRequest) , 0 );
			char commandToBeSent[4096];
			strcpy(commandToBeSent, "");
			strcpy(commandToBeSent, clientHostName);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, clientIpAddress);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, "exec");
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, fileName);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, cfl);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, commandArguments);
			send(sock , commandToBeSent , strlen(commandToBeSent) , 0 );
//			printf("Command Sent = %s\n", commandToBeSent);
			int sizeOfResult;
			read(sock, &sizeOfResult, sizeof(int));

			char *buf;
			buf = (char *)malloc(sizeof(char) * (sizeOfResult + 1));
			read( sock , buf, sizeOfResult);
			printf("result of command = %s\n",buf);

		}
		else
		{
			// error in creating outging CRS request thread
			serv_addr.sin_port = htons(CRS_PORT);
			// Convert IPv4 and IPv6 addresses from text to binary form
			if(inet_pton(AF_INET, serverIpAddress , &serv_addr.sin_addr)<=0) 
			{
				printf("\nInvalid address/ Address not supported \n");
				return NULL;
			}
			if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
			{
				printf("\nConnection Failed \n");
				return NULL;
			}

			char commandToBeSent[4096];
			strcpy(commandToBeSent, "");
			strcpy(commandToBeSent, clientHostName);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, clientIpAddress);
			strcat(commandToBeSent, "#");
			strcat(commandToBeSent, command);
			send(sock , commandToBeSent , strlen(commandToBeSent) , 0 );
			
			char buffer[1024] = {0};
			read( sock , buffer, 1024);
			if(str.find("search") != std::string::npos && (strcmp("FILE NOT FOUND", buffer) != 0))
			{
				int index = 0;
				char temp[256];
				lockOnIndividualResults.lock();
				individualResults.clear();
				for(int counter = 1; counter < strlen(buffer); counter++)
				{
					if(buffer[counter] == '&')
					{
						temp[index] = '\0';
						index = 0;
						individualResults.push_back(temp);
					}
					else
						temp[index++] = buffer[counter];	
				}
				temp[index] = '\0';
				individualResults.push_back(temp);

				lockOnIndividualResults.unlock();

				for(int counter = 0; counter < individualResults.size(); counter++)
				{
					char *token1, *token2, *token3, *token4, *token5;
					char fileName[128], clientHostName[128], clientFileLocation[128], clientIpAddress[128], clientPort[128];
					char resp[512];
					strcpy(resp, individualResults[counter].c_str());
					token1 = strtok(resp, "#");
					if(token1 != NULL)
					{
						strcpy(fileName, token1);
						token2 = strtok(NULL, "#");

						if(token2 != NULL)
						{
							strcpy(clientHostName, token2);	
							token3 = strtok(NULL, "#");

							if(token3 != NULL)
							{
								strcpy(clientFileLocation, token3);	
								token4 = strtok(NULL, "#");

								if(token4 != NULL)
								{
									strcpy(clientIpAddress, token4);	
									token5 = strtok(NULL, "#");

									if(token5 != NULL)
									{
										strcpy(clientPort, token5);	
									}
								}
							}
						}
					}		

					printf(" [%d] %s:%s:%s:%s:%s\n",counter+1, fileName, clientFileLocation, clientHostName, clientIpAddress, clientPort);
				}

			}//if
			else
				printf("Response= %s\n",buffer);
		}
		close(sock);
	} // while

}// end of function

// send heart beats after every 10 seconds
void *handleHeartBeats(void *req)
{
	struct sockaddr_in address;
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	char buffer[1024] = {0};
	HeartBeat *heartBeat = (HeartBeat *)req;

	char heartBeatMessage[256];
	strcpy(heartBeatMessage, "hello");
	strcat(heartBeatMessage, "#");
	strcat(heartBeatMessage, clientHostName);
	strcat(heartBeatMessage, "#");
	strcat(heartBeatMessage, (std::to_string(FILE_DOWNLOAD_PORT)).c_str());
	strcat(heartBeatMessage, "#");
	strcat(heartBeatMessage, clientIpAddress);
	//	printf("started heart beat service successfully\n");
	while(1)
	{

		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			printf("\n Socket creation error \n");
			//getchar();
			return NULL;
		}
		memset(&serv_addr, '0', sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;

		//		printf("sending to %d port \n",CRS_PORT);

		serv_addr.sin_port = htons(CRS_PORT);
		if(inet_pton(AF_INET, serverIpAddress, &serv_addr.sin_addr)<=0)
		{
			printf("\nInvalid address/ Address not supported \n");
			return NULL;
		}
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			printf("\nConnection Failed \n");
			return NULL;
		}
		send(sock , heartBeatMessage , strlen(heartBeatMessage) , 0 );
		//		printf("heart beat sent");	
		sleep(30);
	}

}


// execute remote procedure
void executeRemoteProceduralCallAtClient(char *command, char **arguments)
{


}





