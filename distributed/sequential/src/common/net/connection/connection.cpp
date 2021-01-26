#include <unistd.h> 
#include "connection.h"
#include "cstring"
#include <string>
#include <iostream>

Connection::Connection(int fd)
{
    this->fd = fd;
}

int Connection::Recv(char** dst)
{
    char* messageLength = new char[11];
    read(this->fd , messageLength, 10);
    messageLength[10] = '\0';
    const int length = std::atoi(messageLength);
    (*dst) = new char[length];
    int readLength = read(this->fd , (*dst), length);
    while(readLength < length)
    {
        const int bytesRecieved = read(this->fd , (*dst) + readLength, length - readLength);
        readLength += bytesRecieved;
        // std::cout << "Still waiting for data" << std::endl;
    }
    delete messageLength;
    // (*dst)[length - 1] = '\0';
    return length;
}

void Connection::Send(const char* message, const char flag, const size_t size)
{
    const size_t length = size + 1;
    std::string lengthStr = std::to_string(length);
    for ( int i = lengthStr.length(); i < 10; i++)
    {
        lengthStr = '0' + lengthStr;
    }

    this->writeMutex.lock();
    write(this->fd, lengthStr.c_str(), lengthStr.length());
    write(this->fd, &flag, 1);
    write(this->fd, message, size);
    this->writeMutex.unlock();
}

void Connection::SendMany(const char** messages, const char flag, const size_t* sizes, const int messagesCount)
{
    size_t length = 1;
    for(int i = 0; i < messagesCount; i++)
    {
        length += sizes[i];
    }
    std::string lengthStr = std::to_string(length);
    for ( int i = lengthStr.length(); i < 10; i++)
    {
        lengthStr = '0' + lengthStr;
    }

    this->writeMutex.lock();
    write(this->fd, lengthStr.c_str(), lengthStr.length());
    write(this->fd, &flag, 1);
    for(int i = 0; i < messagesCount; i++)
    {
        write(this->fd, messages[i], sizes[i]);
    }
    this->writeMutex.unlock();
    // delete data;
}

Connection::~Connection()
{
    close(this->fd);
}
