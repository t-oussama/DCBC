#ifndef SOCKET
#define SOCKET

#include <unistd.h> 
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "string.h"

#include "../connection/connection.h"

class Socket {
public:
    Socket();
    Connection* Accept();
    void Bind(const int port);
    void Listen(const int n);
    Connection* Connect(const char* host, const int port);

private:
    int server_fd = -1;
    struct sockaddr_in address;
};
#endif