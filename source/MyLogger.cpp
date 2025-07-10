#include "MyLogger.h"

#include <iomanip>

std::fstream MyLogger::logFile;
std::unique_ptr<MyThreadPool> MyLogger::pool=nullptr;

MyLogger::MyLogger(const std::string& fileName, const bool isDebug) : fileName(fileName), debugMode(isDebug) {
    // Initialize the thread pool with a specified number of workers and maximum jobs
    pool=std::make_unique<MyThreadPool>(4, 100); // Example: 4 workers, max 100 jobs

    SetFilename(fileName);
}

std::unique_ptr<MyLogger, MyLogger::Deleter> MyLogger::Create(const std::string& fileName, const bool isDebug) {
    return std::unique_ptr<MyLogger, MyLogger::Deleter>(new MyLogger(fileName, isDebug));
}

std::string MyLogger::GetLevelString(const LogLevel level) {
    switch(level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

int MyLogger::WriteLog(const LogLevel level, const std::string& message) {
    const std::string logMessage = CurrentTime() + " [" + GetLevelString(level) + "] " + message;

    pool->PushJob(Write(), new std::string(logMessage));
    return 0; // Return 0 for success
}

std::string MyLogger::CurrentTime() {
    auto now=std::chrono::system_clock::now();
    std::time_t now_time_t=std::chrono::system_clock::to_time_t(now);
    std::tm now_tm=*std::localtime(&now_time_t);

    std::ostringstream oss;
    oss<<std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void MyLogger::SetFilename(const std::string& fileName) {
    if (logFile.is_open())logFile.close();
    logFile.open(fileName, std::ios::out | std::ios::app);
    if (!logFile.is_open())
        throw std::runtime_error("Failed to open log file: " + fileName);
}

void (*MyLogger::Write())(void*) {
    return [](void* data) {
        const std::string* logMessage=static_cast<std::string*>(data);
        if(logFile.is_open()) {
            logFile<<*logMessage<<std::endl;
            logFile.flush();
        }
        printf("%s\n", logMessage->c_str());
        delete logMessage; // Clean up the allocated memory
    };
}
