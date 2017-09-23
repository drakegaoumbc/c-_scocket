#include <iostream>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <mutex>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MAXCLIENTS 3
#define PORT 4203

using namespace std;
std::mutex mu;

void newClient(int client_fd, string clientName, int* clients, int server_fd);
bool isclosed(int sock);

int main() {
	freopen("logFile.txt", "w", stderr);
	int server_fd, opt = 1;
	socklen_t len;
	struct sockaddr_in serverAddr, clientAddr;
	int clients[MAXCLIENTS];
	for(int i = 0; i < MAXCLIENTS; i++) {
		clients[i] = 0;
	}
	// create socket
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cerr << "socket err" << endl;
		exit(EXIT_FAILURE);
	}

	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, (socklen_t) sizeof(opt))) {
		cerr << "Setsockopt error" << endl;
		exit(EXIT_FAILURE);
	}


	memset(&serverAddr, '0', sizeof(serverAddr));
	// bind address and port
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_family = AF_INET;

	if(bind(server_fd, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
		cerr << "binding error" << endl;
		exit(EXIT_FAILURE);
	}

	// listening
	if(listen(server_fd, MAXCLIENTS) < 0) {
		cerr << "listen error" << endl;
		exit(EXIT_FAILURE);
	}

	int max = 0;
	int clientAddrSize = sizeof(clientAddr);

	while(true) {

		cout << "Waiting for Connection..." << endl;

		int client_fd = accept(server_fd, (struct sockaddr*) &clientAddr, (socklen_t*) &clientAddrSize);
		if(client_fd < 0) {
			cerr << "new sock fail" << endl;
			exit(EXIT_FAILURE);
		}

		if(client_fd == 0)
			continue;

		// add the client array
		for(int i = 0; i < MAXCLIENTS; i++) {
			if(clients[i] == 0) {
				clients[i] = client_fd;
				break;
			}
		}

		getpeername(client_fd, (struct sockaddr*)&clientAddr, (socklen_t*)&clientAddrSize);
		ostringstream ss;
		ss << ntohs(clientAddr.sin_port);
		string from;
		from = ss.str();

		//inet_ntoa(clientAddr.sin_addr);
		cout << "HI: " << client_fd << " " << from << " at port " << ntohs(clientAddr.sin_port) << endl;

		// create new thread for client to recv
		thread t(newClient, client_fd, from, clients, server_fd);
		t.detach();
	}
	return 0;
}

/* client thread will handle io operation */
void newClient(int client_fd, string clientName, int* clients, int server_fd) {
	char buffer[4096] = {0};
	while(!isclosed(client_fd)) {
	  // if the same client from client side send msg to correspond client thread in server side
		int bytesIn = recv(client_fd, buffer, sizeof(buffer), 0);
		cout << bytesIn << endl;
		char msgtoSend[4096];

		strcpy(msgtoSend, clientName.c_str());
		msgtoSend[clientName.length()] = ' ';
		strcat(msgtoSend, buffer);
		msgtoSend[clientName.length() + bytesIn + 1] = '\0';

		// broadcast to all existign clients except self
		for(int i = 0; i < MAXCLIENTS; i++) {
			if(clients[i] != client_fd && clients[i] != 0) {
				send(clients[i], msgtoSend, sizeof(msgtoSend), 0);
			}
		}
		memset(buffer, 0, 4096);
		memset(msgtoSend, 0, sizeof(msgtoSend));
	}

	shutdown(client_fd, 2);
}

bool isclosed(int sock) {
  fd_set rfd;
  FD_ZERO(&rfd);
  FD_SET(sock, &rfd);
  timeval tv = { 0 };
  select(sock+1, &rfd, 0, 0, &tv);
  if (!FD_ISSET(sock, &rfd))
    return false;
  int n = 0;
  ioctl(sock, FIONREAD, &n);
  return n == 0;
}


/*char msgtoSend[4096];
strcpy(msgtoSend, clientName.c_str());
msgtoSend[clientName.length()] = ' ';
strcat(msgtoSend, buffer);
msgtoSend[clientName.length() + bytesIn + 1] = '\0';
send(clients[i], msgtoSend, sizeof(msgtoSend), 0);*/



