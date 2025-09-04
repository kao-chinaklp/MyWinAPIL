#ifndef MYSOCKETX_H
#define MYSOCKETX_H

#include <memory>
#include <winsock2.h>
#include <string>

#include "MyLogger.h"

enum class ProtocolType {
    TCP,
    UDP
};

enum class IPType {
    IPv4,
    IPv6
};

enum class SocketType {
    server,
    client
};

enum class ProcessState {
    AUTH,
    PROCESS,
    CLOSE
};

class MySocketX {
    public:
        explicit MySocketX(std::shared_ptr<MyLogger> logger=nullptr);
        ~MySocketX();

        static bool Initialize();
        static bool Create(ProtocolType protocolType, const std::string& IP, unsigned port,
            SocketType socketType, IPType ipType=IPType::IPv4);
        static bool Start(void (*Function)(void *), void* data=nullptr);
        static void Close();

    private:
        static void Work(void* data);

    public:
        typedef struct {
            SOCKET socket;
        }PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

        typedef struct {
            OVERLAPPED overlapped;
            WSABUF wsabuf;
            char* buffer=new char[1024];
            DWORD bytesReceived;
            DWORD bytesSent;
            SOCKET socket;
            std::string accumulatedData;
            ProcessState state=ProcessState::AUTH;
        }PER_IO_DATA, *LPPER_IO_DATA;

        static const std::string eof;

    private:
        class MySocketXImpl;
        static inline std::unique_ptr<MySocketXImpl> impl;
};

#endif //MYSOCKETX_H
