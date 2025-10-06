#ifndef MYSOCKETX_H
#define MYSOCKETX_H

#include <memory>
#include <winsock2.h>
#include <string>

#include "MyLogger.h"

#define DATA_SIZE 1024

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
    DEFAULT,
    SEND,
    RECEIVE,
    CLOSE
};

typedef ui ClientID;

class MySocketX {
    public:
        explicit MySocketX(const std::shared_ptr<MyLogger>& logger=nullptr);
        ~MySocketX();

        static bool Initialize();
        static bool Create(ProtocolType protocolType, const std::string& IP, unsigned port,
            SocketType socketType, IPType ipType=IPType::IPv4);
        bool Start(void* extraData=nullptr);
        void SaveClientInfo(SOCKET sock, void* extraData=nullptr);
        static bool SendTo(const std::string& data, ClientID id=0);
        static void BroadCast(const std::string& data);
        static void Close();

        virtual void OnConnect(SOCKET sock, void* data);
        virtual bool OnSend(SOCKET sock, void* data);
        virtual bool OnReceive(SOCKET sock, void* data);
        virtual void Process(); // For client

    private:
        static void Work(void* data);

    public:
        typedef struct {
            SOCKET socket;
        }PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

        typedef struct {
            WSAOVERLAPPED overlapped;
            WSABUF wsabuf;
            char* buffer=new char[DATA_SIZE];
            DWORD bytesReceived;
            DWORD bytesSent;
            SOCKET socket;
            std::string accumulatedData;
            ProcessState state=ProcessState::DEFAULT;
            ClientID clientId;
        }PER_IO_DATA, *LPPER_IO_DATA;

        static const std::string eof;

    private:

        class MySocketXImpl;
        struct Deleter {
            void operator()(const MySocketXImpl* p) const;
        };
        static inline std::unique_ptr<MySocketXImpl, Deleter> impl;
};

#endif //MYSOCKETX_H
