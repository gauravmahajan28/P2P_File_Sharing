all: server client

server: serverActionHandling.o server.o serverFileHandling.o
	g++ serverActionHandling.o server.o serverFileHandling.o -lpthread -o  server


server.o: server.cpp
	g++ -c -Wall -std=c++0x server.cpp -lpthread

serverActionHandling.o: serverActionHandling.cpp serverActionHandling.h
	g++ -c -Wall -std=c++0x serverActionHandling.cpp -lpthread

serverFileHandling.o: serverFileHandling.cpp serverFileHandling.h
	g++ -c -Wall -std=c++0x serverFileHandling.cpp



client: clientActionHandling.o client.o
	g++ clientActionHandling.o client.o -o client -lpthread

clientActionHandling.o : clientActionHandling.cpp clientActionHandling.h
	g++ -c -Wall -std=c++0x  clientActionHandling.cpp -lpthread

client.o: client.cpp
	g++ -c -Wall -std=c++0x client.cpp -lpthread

	
