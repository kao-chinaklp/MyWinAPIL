#include "MyWindowX.h"

std::shared_ptr<MyLogger> logger=nullptr;

// void test(UINT uMsg){
//     MyLogger::WriteLog(LogLevel::Info, "test");
// }
void* Run(void* data){
    for (int i=0;i<1000;i++)
        MyLogger::WriteLog(LogLevel::Info, "test");
    return nullptr;
}

int main(){
    // auto* window=new MyWindowX("default", "test", test);
    // window->show(SW_NORMAL);

    pthread_t thread;
    logger=MyLogger::Create("log.txt");
    pthread_create(&thread, nullptr, Run, nullptr);
    pthread_join(thread, nullptr);
    return 0;
}