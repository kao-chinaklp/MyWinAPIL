#include "MySocketX.h"

#include <vector>
#include <ws2tcpip.h>

typedef struct {
    HANDLE* iocp;
    void (*Process)(void *);
}ConnectionData;

typedef struct {
    std::string message;
    MySocketX::LPPER_HANDLE_DATA handleData;
}UserData;

class MySocketX::MySocketXImpl {
    public:
        explicit MySocketXImpl(std::shared_ptr<MyLogger> logger) {
            if (this->logger==nullptr)
                this->logger=MyLogger::Create("log.txt");
            else
                this->logger=std::move(logger);

            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            maxWorkers=sysInfo.dwNumberOfProcessors * 2;
            threadPool=std::make_unique<MyThreadPool>(maxWorkers, 100);// 最大线程数为CPU核心数的两倍

            listenSocket=INVALID_SOCKET;
            clientSocket=INVALID_SOCKET;
            socketType=SocketType::client;
            ipType=IPType::IPv4;
        }

        ~MySocketXImpl()=default;

        WSAData& getWSAData() {return wsaData;}
        HANDLE& getIOCP() {return iocp;}
        SOCKET& getListenSocket() {return listenSocket;}
        SOCKET& getClientSocket() {return clientSocket;}
        SOCKADDR_IN& getServerAddr() {return serverAddr;}
        SOCKADDR_IN& getClientAddr() {return clientAddr;}
        SOCKADDR_IN6& getServerAddr6() {return serverAddr6;}
        SOCKADDR_IN6& getClientAddr6() {return clientAddr6;}

        SocketType& getSocketType()  {return socketType;}
        IPType& getIPType() {return ipType;}

        void Log(const LogLevel level, const std::string& msg) {logger->WriteLog(level, msg);}
        void StartThread(void (*Function)(void*), void (*Process)(void*)) {
            connections.resize(maxWorkers, {&iocp, Process});
            for (int i=1;i<=maxWorkers;++i)
                threadPool->PushJob(Function, &(connections[i-1]));
        }

    private:
        std::unique_ptr<MyThreadPool> threadPool;
        std::shared_ptr<MyLogger> logger;

        WSAData wsaData{};
        HANDLE iocp{};
        SOCKET listenSocket;
        SOCKET clientSocket;
        SOCKADDR_IN serverAddr{};
        SOCKADDR_IN clientAddr{};
        SOCKADDR_IN6 serverAddr6{};
        SOCKADDR_IN6 clientAddr6{};

        SocketType socketType;
        IPType ipType;

        unsigned maxWorkers;
        std::vector<ConnectionData> connections;
};

const std::string MySocketX::eof = "\r\n\r\n"; // 结束符

MySocketX::MySocketX(std::shared_ptr<MyLogger> logger) {
    impl=std::make_unique<MySocketXImpl>(std::move(logger));
}

MySocketX::~MySocketX() {
    Close();
}

bool MySocketX::Initialize() {
    // 初始化
    if (WSAStartup(MAKEWORD(2, 2), &impl->getWSAData())!=0) {
        impl->Log(LogLevel::Fatal, "WSAStartup failed: "+std::to_string(WSAGetLastError()));

        return false;
    }
    impl->Log(LogLevel::Debug, "Socket initialized successfully.");

    return true;
}

bool MySocketX::Create(const ProtocolType protocolType, const std::string& IP, unsigned port,
    const SocketType socketType, const IPType ipType) {

    // 服务端实现
    if (socketType==SocketType::server) {
        auto& iocp=impl->getIOCP();
        iocp=CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        if (iocp==nullptr) {
            impl->Log(LogLevel::Fatal, "Creating IoCompletionPort failed: "+std::to_string(GetLastError()));
            return false;
        }

        SOCKET& listenSocket=impl->getListenSocket();
        listenSocket=WSASocketA((ipType==IPType::IPv4)?AF_INET:AF_INET6,
            (protocolType==ProtocolType::TCP)?SOCK_STREAM:SOCK_DGRAM, 0,
            nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (listenSocket==INVALID_SOCKET) {
            impl->Log(LogLevel::Fatal, "WSASocket failed: "+std::to_string(WSAGetLastError()));
            return false;
        }
        impl->Log(LogLevel::Debug, "Socket created successfully.");

        // 绑定地址
        if (ipType==IPType::IPv4) {
            auto& serverAddr=impl->getServerAddr();
            serverAddr.sin_family=AF_INET;
            serverAddr.sin_addr.s_addr=htonl(INADDR_ANY);
            serverAddr.sin_port=htons(port);
            inet_pton(AF_INET, IP.c_str(), &serverAddr.sin_addr);

            if (bind(listenSocket, reinterpret_cast<SOCKADDR *>(&serverAddr), sizeof(serverAddr))==SOCKET_ERROR) {
                impl->Log(LogLevel::Fatal, "bind failed: "+std::to_string(WSAGetLastError()));
                return false;
            }

            impl->Log(LogLevel::Info, "Socket bound successfully("+IP+":"+std::to_string(port)+").");
        }

        if (ipType==IPType::IPv6) {
            auto& serverAddr6=impl->getServerAddr6();
            serverAddr6.sin6_family=AF_INET6;
            serverAddr6.sin6_addr=in6addr_any;
            serverAddr6.sin6_port=htons(port);
            inet_pton(AF_INET6, IP.c_str(), &serverAddr6.sin6_addr);

            if (bind(impl->getListenSocket(), reinterpret_cast<SOCKADDR *>(&serverAddr6), sizeof(serverAddr6))==SOCKET_ERROR) {
                impl->Log(LogLevel::Fatal, "bind failed: "+std::to_string(WSAGetLastError()));
                return false;
            }

            impl->Log(LogLevel::Info, "Socket bound successfully("+IP+":"+std::to_string(port)+").");
        }
    }

    // 客户端实现
    if (socketType==SocketType::client) {
        if (ipType==IPType::IPv4) {
            auto& clientSocket=impl->getClientSocket();
            auto& clientAddr=impl->getClientAddr();
            clientSocket=socket(AF_INET, (protocolType==ProtocolType::TCP)?SOCK_STREAM:SOCK_DGRAM, 0);
            if (clientSocket==INVALID_SOCKET) {
                impl->Log(LogLevel::Fatal, "socket failed: "+std::to_string(WSAGetLastError()));
                return false;
            }

            clientAddr.sin_family=AF_INET;
            clientAddr.sin_addr.s_addr=inet_addr(IP.c_str());
            clientAddr.sin_port=htons(port);
        }
        if (ipType==IPType::IPv6) {
            auto& clientSocket=impl->getClientSocket();
            auto& clientAddr6=impl->getClientAddr6();
            clientSocket=socket(AF_INET6, (protocolType==ProtocolType::TCP)?SOCK_STREAM:SOCK_DGRAM, 0);
            if (clientSocket==INVALID_SOCKET) {
                impl->Log(LogLevel::Fatal, "socket failed: "+std::to_string(WSAGetLastError()));
                return false;
            }

            clientAddr6.sin6_family=AF_INET6;
            inet_pton(AF_INET6, IP.c_str(), &clientAddr6.sin6_addr);
            clientAddr6.sin6_port=htons(port);
        }
    }
    impl->Log(LogLevel::Debug, "Socket created successfully.");

    impl->getSocketType()=socketType;
    impl->getIPType()=ipType;

    return true;
}

bool MySocketX::Start(void (*Function)(void *), void* data) {
    // 服务端实现
    if (impl->getSocketType()==SocketType::server) {
        auto& clientSocket=impl->getClientSocket();
        auto& listenSocket=impl->getListenSocket();
        int clientAddrSize=sizeof(clientSocket);
        LPPER_HANDLE_DATA handleData;
        LPPER_IO_DATA lpIoData;
        HANDLE completionPort=impl->getIOCP();
        DWORD flags=0;

        impl->StartThread(Work, Function);

        // 监听端口
        if (listen(impl->getListenSocket(), SOMAXCONN)==SOCKET_ERROR) {
            impl->Log(LogLevel::Fatal, "listen failed: "+std::to_string(WSAGetLastError()));
            return false;
        }
        impl->Log(LogLevel::Info, "Listening for connections...");

        while (true) {
            // 接受连接
            if (impl->getIPType()==IPType::IPv4)
                clientSocket=accept(listenSocket, reinterpret_cast<SOCKADDR*>(&impl->getClientAddr()), &clientAddrSize);
            if (impl->getIPType()==IPType::IPv6)
                clientSocket=accept(listenSocket, reinterpret_cast<SOCKADDR*>(&impl->getClientAddr6()), &clientAddrSize);

            if (clientSocket==INVALID_SOCKET) {
                impl->Log(LogLevel::Error, "accept failed: "+std::to_string(WSAGetLastError()));
                continue; // 继续等待连接
            }

            // 关联IOCP
            handleData=static_cast<LPPER_HANDLE_DATA>(malloc(clientAddrSize));
            handleData->socket=clientSocket;

            CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), completionPort, reinterpret_cast<ULONG_PTR>(handleData), 0);

            // 初始化缓冲区
            lpIoData=static_cast<LPPER_IO_DATA>(malloc(sizeof(PER_IO_DATA)));
            ZeroMemory(&(lpIoData->overlapped), sizeof(WSAOVERLAPPED));
            lpIoData->wsabuf.buf=lpIoData->buffer;
            lpIoData->wsabuf.len=sizeof(lpIoData->buffer);
            lpIoData->bytesReceived=0;
            lpIoData->bytesSent=0;
            lpIoData->socket=clientSocket;
            flags=0;

            // 投递接收请求
            if (WSARecv(clientSocket, &(lpIoData->wsabuf), 1, &(lpIoData->bytesReceived), &flags,
                &(lpIoData->overlapped), nullptr)==SOCKET_ERROR) {
                if (WSAGetLastError()!=WSA_IO_PENDING) {
                    impl->Log(LogLevel::Error, "WSARecv failed: "+std::to_string(WSAGetLastError()));
                    closesocket(clientSocket);
                    free(handleData);
                    free(lpIoData);
                    closesocket(clientSocket);
                }
            }
        }
    }

    // 客户端实现
    if (impl->getSocketType()==SocketType::client) {
        // 尝试连接
        unsigned Count=0;
        while(++Count<=3) {
            impl->Log(LogLevel::Info, "Trying to connect("+std::to_string(++Count)+")...");

            if (connect(impl->getClientSocket(),
                (impl->getIPType()==IPType::IPv4)?reinterpret_cast<SOCKADDR*>(&impl->getClientAddr()):reinterpret_cast<SOCKADDR*>(&impl->getClientAddr6()),
                (impl->getIPType()==IPType::IPv4)?sizeof(SOCKADDR_IN):sizeof(SOCKADDR_IN6))==SOCKET_ERROR) {
                impl->Log(LogLevel::Error, "connect failed: "+std::to_string(WSAGetLastError()));

                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }

        if (Count>3) {
            impl->Log(LogLevel::Error, "Failed to connect after 3 attempts.");
            return false;
        }
        impl->Log(LogLevel::Info, "Connected successfully.");

        // 数据处理（自定义）
        Function(data);
    }

    return true;
}

void MySocketX::Close() {
    impl->Log(LogLevel::Info, "Closing socket...");

    if (impl->getIOCP()!=nullptr) {
        CloseHandle(impl->getIOCP());
        impl->getIOCP()=nullptr;
    }

    if (impl->getListenSocket()!=INVALID_SOCKET) {
        closesocket(impl->getListenSocket());
        impl->getListenSocket()=INVALID_SOCKET;
    }

    if (impl->getClientSocket()!=INVALID_SOCKET) {
        closesocket(impl->getClientSocket());
        impl->getClientSocket()=INVALID_SOCKET;
    }

    WSACleanup();
}

void MySocketX::Work(void* data) {
    // Demo
    auto* connectionData=static_cast<ConnectionData *>(data);

    HANDLE completionPort=connectionData->iocp;
    DWORD transferredBytes;
    ULONG_PTR completionKey;
    LPPER_IO_DATA lpIoData;
    LPPER_HANDLE_DATA handleData;
    uint32_t dataSize=0;

    while (true) {
        const BOOL ok=GetQueuedCompletionStatus(completionPort, &transferredBytes,
            &completionKey, reinterpret_cast<LPOVERLAPPED *>(&lpIoData), INFINITE);

        handleData=reinterpret_cast<LPPER_HANDLE_DATA>(completionKey);

        if (lpIoData->bytesReceived<4)continue;

        if (!ok) {
            // 连接错误或关闭
            impl->Log(LogLevel::Error, "GetQueuedCompletionStatus failed: "+std::to_string(WSAGetLastError()));
            closesocket(handleData->socket);
            free(handleData);
            free(lpIoData);
            continue;
        }

        if (transferredBytes==0) {
            // 客户端关闭
            impl->Log(LogLevel::Info, "Client disconnected.");
            closesocket(handleData->socket);
            free(handleData);
            free(lpIoData);
            continue;
        }

        lpIoData->accumulatedData.insert(
            lpIoData->accumulatedData.end(),
            lpIoData->buffer,
            lpIoData->buffer + transferredBytes);

        bool continueProcessing=true;
        while (continueProcessing) {
            switch (lpIoData->state) {
                case ProcessState::AUTH:
                    // 这里可以添加认证逻辑
                    lpIoData->state=ProcessState::PROCESS;
                    break;

                case ProcessState::PROCESS:
                    if (lpIoData->accumulatedData.size()<4) break;

                    // 处理数据
                    memcpy(&dataSize, lpIoData->accumulatedData.data(), 4);
                    dataSize=ntohl(dataSize); // 网络字节序转主机字节序

                    // 检查消息完整性
                    if (lpIoData->accumulatedData.size()<dataSize+4) break;

                    // 处理完整数据
                    UserData userData={lpIoData->accumulatedData, handleData};
                    connectionData->Process(&userData); // 自定义处理函数

                    // 移除已处理的数据
                    lpIoData->accumulatedData.erase(
                        lpIoData->accumulatedData.begin(),
                        lpIoData->accumulatedData.begin()+dataSize + 4);
                    break;

                case ProcessState::CLOSE:
                    // 客户端关闭
                    impl->Log(LogLevel::Info, "Client disconnected.");
                    continueProcessing=false;
                    closesocket(handleData->socket);
                    free(handleData);
                    free(lpIoData);
                    break;
            }

            if (lpIoData->state==ProcessState::PROCESS&&lpIoData->accumulatedData.size()<4)
                continueProcessing=false;
        }

        ZeroMemory(&(lpIoData->overlapped), sizeof(WSAOVERLAPPED));
        lpIoData->wsabuf.buf=lpIoData->buffer;
        lpIoData->wsabuf.len=sizeof(lpIoData->buffer);

        DWORD flags = 0;
        DWORD bytesReceived = 0;

        if (WSARecv(handleData->socket, &(lpIoData->wsabuf), 1, &bytesReceived,
                    &flags, &(lpIoData->overlapped), nullptr) == SOCKET_ERROR)
            if (WSAGetLastError()!=WSA_IO_PENDING) {
                impl->Log(LogLevel::Error, "WSARecv failed: "+std::to_string(WSAGetLastError()));
                closesocket(handleData->socket);
                free(handleData);
                free(lpIoData);
            }
    }
}
