#include <iostream>
#include "serverFileHandling.h"
#include<unordered_map>
#include<vector>
#include<string>
#include <unistd.h>

// debugging function to print map
void printMap(std::unordered_map<std::string, std::vector< std::string > > firstMap, std::unordered_map<std::string, std::vector< std::string > > secondMap)
{
	std::unordered_map<std::string, std::vector< std::string> >::iterator firstMapIt, secondMapIt;

	if(firstMap.empty())
	{
		printf("first map is empty\n");
	}
	else
	{
		 for (  firstMapIt = firstMap.begin(); firstMapIt != firstMap.end(); ++firstMapIt )
		 {
		     std::cout << " " << firstMapIt->first << ":\n";

		     for(unsigned int i = 0; i < (firstMapIt->second).size(); i++)
		     {
			std::cout<< " - " << firstMapIt->second[i] << " - ";
		     }
		     printf("\n");
		 }
		 std::cout << std::endl;
	}
	
	if(secondMap.empty())
	{
		printf("second map is empty\n");
	}
	else
	{
		 for (  secondMapIt = secondMap.begin(); secondMapIt != secondMap.end(); ++secondMapIt )
		 {    
		     std::cout << " " << secondMapIt->first << ":\n";  // cannot modify *it
		     for(unsigned int i = 0; i < (secondMapIt->second).size(); i++)
		     {
			std::cout<< " - " << secondMapIt->second[i] << " - ";
		     }
		     printf("\n");
		}     
		 std::cout << std::endl;
	}

}

// function to read server mapping file
FileLocationMapping *readServerMappingFile(FILE *mappingFile)
{
	
	// setting file pointer to start
	rewind(mappingFile);

	FileLocationMapping *fileLocationMapping;
	fileLocationMapping = (FileLocationMapping *)malloc(sizeof(FileLocationMapping));
	//mapping from file name to list of hosts and locations
	std::unordered_map<std::string, std::vector< std::string > > fileNameClientLocationMapping;
	// mapping from client ip address 
	std::unordered_map<std::string, std::vector< std::string> > clientIpAddressToFileNames;

	std::unordered_map<std::string, std::vector< std::string> >::iterator fileNameToClientMappingIterator, clientIpToFileNamesIterator;

	// couting number of lines
	int count = 0;
	char c;
	for (c = getc(mappingFile); c != EOF; c = getc(mappingFile))
		if (c == '\n') // Increment count if this character is newline
			count = count + 1;
	
	rewind(mappingFile);

	// read file line by line
	while(count >= 1)
	{
		// read from file line by line

		char fileName[128], clientFileLocation[128], clientHostName[128], entriesToBeAdded[512], entriesToBeAddedSecond[512];

		fileNameToClientMappingIterator = fileNameClientLocationMapping.begin();
		clientIpToFileNamesIterator = clientIpAddressToFileNames.begin();
		char line[1024];
	//	fscanf(mappingFile, "%s",line);
		fgets(line, 1024, mappingFile);

		int id = 0;
		char temp[512];
		int index = 0;
		for(int counter = 0; counter < strlen(line); counter++)
		{
			if(line[counter] == ':')
			{
				temp[index] = '\0';

				if(id == 0)
					strcpy(fileName, temp);
				if(id == 1)
					strcpy(clientFileLocation, temp);
				if(id == 2)
					strcpy(clientHostName, temp);
				
				index = 0;
				id++;
			}
			else
				temp[index++] = line[counter];
		}
		temp[index-1] = '\0';
		strcpy(clientHostName, temp);
		
		strcpy(entriesToBeAdded, "");
		strcat(entriesToBeAdded, clientFileLocation);
		strcat(entriesToBeAdded, "#");
		strcat(entriesToBeAdded, clientHostName);

		std::string key1(fileName), value1(entriesToBeAdded);
		if(fileNameClientLocationMapping.find(key1) == fileNameClientLocationMapping.end())
		{
			//			printf("not already in first map\n");
			std::vector<std::string> clientFileLocations;
			clientFileLocations.push_back(value1);
			fileNameClientLocationMapping[key1] =  clientFileLocations;

		}
		// file name entry is already present
		else
		{

			//			printf("already in first map\n");
			std::vector<std::string> clientFileLocations = fileNameClientLocationMapping.find(key1)->second;
			clientFileLocations.push_back(value1);
			fileNameClientLocationMapping[key1] =  clientFileLocations;

		}
		count--;
	}
	fileLocationMapping->fileNameToClientIp_ClientLocation = fileNameClientLocationMapping;
	fileLocationMapping->clientIpAddressToFileNames = clientIpAddressToFileNames;
	return fileLocationMapping;

}

//log client request
void logClientRequest(struct tm *now, char *clientIpAddress,  char *clientHostName, char *action, char *fileName, char *fileLocation ,FILE *fp)
{
	fprintf(fp, "%d-%d-%d %s %s %s-%s-%s\n", now->tm_hour, now->tm_min, now->tm_sec, clientHostName, clientIpAddress, action, fileName, fileLocation);

}


//update file location mapping
void updateClientFileMapping(FileLocationMapping *fileLocationMapping, char *fileMappingName)
{
	FILE *mappingFile;
	mappingFile = fopen(fileMappingName, "w");
	
	std::unordered_map<std::string, std::vector< std::string> >::iterator mapIterator;


	for (  mapIterator = (fileLocationMapping->fileNameToClientIp_ClientLocation).begin(); mapIterator !=  (fileLocationMapping->fileNameToClientIp_ClientLocation).end(); ++mapIterator )
	{

		char fileName[128];
		strcpy(fileName, (mapIterator->first).c_str());
		for(unsigned int counter = 0; counter < (mapIterator->second).size(); counter++)
		{
			char request[512];
			strcpy(request, (mapIterator->second)[counter].c_str());

			char *tokens = strtok(request, "#");
			char *token2, *token3;
			char clientIpAddress[128];
			char clientHostName[128];
			char clientFileLocation[128];
			strcpy(clientIpAddress, "");
			strcpy(clientHostName, "");
			strcpy(clientFileLocation, "");
			while(tokens != NULL)
			{
				strcpy(clientFileLocation, tokens);
				token2= strtok(NULL, "#");
				if(token2 == NULL)
					break;
				strcpy(clientHostName, token2);
			}
			fprintf(mappingFile, "%s:%s:%s\n",fileName, clientFileLocation, clientHostName);
		}// for
	}// for
	fclose(mappingFile);
}


//update available client list
void updateAvailableClientList(char *clientIpAddress, char *clientHostName, ClientStatus clientStatus)
{

}



