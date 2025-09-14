#include "MyWindowX.h"

std::shared_ptr<MyLogger> logger(nullptr, MyLogger::Deleter());

// void test(UINT uMsg){
//     MyLogger::WriteLog(LogLevel::Info, "test");
// }

int main(){
    // auto* window=new MyWindowX("default", "test", test);
    // window->show(SW_NORMAL);
    logger=MyLogger::Create();
    MyLogger::SetFilename("log.txt");
    for (int i=0;i<1000;i++)
        MyLogger::WriteLog(LogLevel::Info, "test");
    return 0;
}