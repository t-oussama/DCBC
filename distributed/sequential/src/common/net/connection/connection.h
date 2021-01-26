#ifndef CONNECTION
#define CONNECTION

#include <mutex>

class Connection {
    public:
        Connection(int fd);
        int Recv(char** dst);
        void Send(const char* message, const char flag, const size_t size);
        void SendMany(const char** messages, const char flag, const size_t* sizes, const int messagesCount);
        ~Connection();

    private:
        int fd;
        std::mutex writeMutex;
};
#endif