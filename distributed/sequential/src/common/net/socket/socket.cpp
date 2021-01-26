#include <iostream>
// Socket dependencies
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "string.h"

#include "socket.h"

Socket::Socket()
{
    // Start-up socket server
    this->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    this->address.sin_family = AF_INET;
}

void Socket::Bind(const int port)
{
    int opt = 1;
    setsockopt(this->server_fd , SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    this->address.sin_addr.s_addr = INADDR_ANY;
    this->address.sin_port = htons(port);
    bind(this->server_fd, (struct sockaddr*)&this->address, sizeof(this->address));
}

void Socket::Listen(const int n)
{
    listen(this->server_fd, n);
}

Connection* Socket::Connect(const char* host, const int port)
{
    this->address.sin_port = htons(port);
    inet_pton(AF_INET, host, &this->address.sin_addr);
    while (connect(this->server_fd, (struct sockaddr *)&this->address, sizeof(this->address)) < 0)
    {
        std::cout << "connection failed" << std::endl;
    }
    return new Connection(this->server_fd);
}

Connection* Socket::Accept()
{
    int addrlen = sizeof(this->address);
    int new_socket = accept(this->server_fd, (struct sockaddr *)&this->address, (socklen_t*)&addrlen);
    return new Connection(new_socket);
}
