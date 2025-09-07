#ifndef MYLOGGER_H
#define MYLOGGER_H

#include "Queue.h"
#include "MyThreadPool.h"

#include <string>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class MyLogger {
    public:
        struct Deleter {
            void operator()(MyLogger* ptr) const {fclose(logFile); delete ptr;}
        };
        friend struct Deleter;
        MyLogger(const MyLogger&)=delete;
        MyLogger& operator=(const MyLogger&)=delete;

    public:
        static std::unique_ptr<MyLogger, Deleter> Create(const std::string& fileName, bool isDebug=false);
        static std::string GetLevelString(LogLevel level);
        static int WriteLog(LogLevel level, const std::string& message);

    private:
        explicit MyLogger(const std::string& fileName, bool isDebug); // Private constructor to prevent instantiation
        ~MyLogger()=default; // Private destructor

        static std::string CurrentTime();
        static void SetFilename(const std::string& fileName);

        static void Write(void* data); // Function to write log messages to file

    private:
        static std::unique_ptr<MyThreadPool> pool;
        static Queue<std::string> logQueue; // Thread-safe queue for log messages
        std::string fileName;
        static FILE* logFile;
        bool debugMode;
};

#endif //MYLOGGER_H