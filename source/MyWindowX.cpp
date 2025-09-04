#include "MyWindowX.h"

class MyWindowX::MyWindowXImpl {
    public:
        MyWindowXImpl(LPCSTR className, LPCSTR windowName, HINSTANCE instance,
            std::shared_ptr<MyLogger> logger)
            : className(className), windowName(windowName), hInstance(instance) {
            hwnd=nullptr;
            if (logger==nullptr)
                this->logger=MyLogger::Create("log.txt");
            else
                this->logger=std::move(logger);
        }

        ~MyWindowXImpl()=default;

        HWND& getHandle() {return hwnd;}
        HINSTANCE& getInstance() {return hInstance;}
        LPCSTR& getClassName() {return className;}
        LPCSTR& getWindowName() {return windowName;}
        void getJob(void (*func)(UINT uMsg)) {Function=func;}

        void setMessage(UINT message) {uMsg=message;}

        void log(const LogLevel level, const std::string& msg) const {MyLogger::WriteLog(level, msg);}

        void runJob() {Function(uMsg);}

    private:
        HWND hwnd; // 窗口句柄
        HINSTANCE hInstance; //程序实例句柄
        LPCSTR className; // 窗口类名
        LPCSTR windowName; // 窗口标题

        std::shared_ptr<MyLogger> logger;

        void (*Function)(UINT uMsg); // 用户自定义的窗口过程函数
        UINT uMsg{};
};

MyWindowX::MyWindowX(LPCSTR className, LPCSTR windowName, void (*Job)(UINT), HINSTANCE instance, std::shared_ptr<MyLogger> logger,
            HWND parent, LPVOID data, int width, int height, int x, int y) {
    if (instance==nullptr)instance=GetModuleHandleA(nullptr);

    impl=std::make_unique<MyWindowXImpl>(className, windowName, instance, std::move(logger));
    impl->getJob(Job);

    // 注册窗口类
    Register();

    // 创建窗口
    Create(impl->getClassName(), impl->getWindowName(), impl->getInstance(), parent,
        data, width, height, x, y);
}

MyWindowX::~MyWindowX() {
    // 销毁窗口
    if(impl->getHandle()!=nullptr)Destroy();

    // 注销窗口类
    Unregister();
}

void MyWindowX::Create(LPCSTR className, LPCSTR windowName, HINSTANCE instance,
                       HWND parent, LPVOID data, int width, int height, int x, int y) const {
    HWND tmp = CreateWindowEx(
        0, className, windowName, WS_OVERLAPPEDWINDOW,
        x, y, width, height, parent, nullptr, instance, const_cast<MyWindowX*>(this)
    );

    impl->getHandle()=tmp;

    if(impl->getHandle()==nullptr) {
        impl->log(LogLevel::Fatal, "Failed to create window: " + std::to_string(GetLastError()));
        MessageBoxEx(nullptr, "Failed to create window", "Fatal", MB_OK | MB_ICONERROR, 0);
    }
    else
        impl->log(LogLevel::Info, "Window created successfully.");
}

void MyWindowX::Destroy() {
    if (impl->getHandle()!=nullptr) {
        impl->log(LogLevel::Debug, "Destroying window.");
        DestroyWindow(impl->getHandle());
        impl->getHandle()=nullptr; // 清空句柄
    }
}

void MyWindowX::ChangeIcon(const std::string& iconName){
    SendMessage(impl->getHandle(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadImage(nullptr, iconName.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE)));
    SendMessage(impl->getHandle(), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(LoadImage(nullptr, iconName.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE)));
}

HWND MyWindowX::getHandle() const {
    return impl->getHandle();
}

HINSTANCE MyWindowX::getInstance() const {
    return impl->getInstance();
}

LPCSTR MyWindowX::getClassName() const {
    return impl->getClassName();
}

LPCSTR MyWindowX::getWindowName() const {
    return impl->getWindowName();
}

void MyWindowX::setHandle(HWND handle) {
    impl->getHandle()=handle;
}

void MyWindowX::getJob(void (*func)(UINT uMsg)) {
    impl->getJob(func);
}

void MyWindowX::show(int nCmdShow) const {
    if (impl->getHandle()!=nullptr) {
        ShowWindow(impl->getHandle(), nCmdShow);
        UpdateWindow(impl->getHandle());
    }
}

void MyWindowX::Register() const {
    WNDCLASSEX wc={};
    wc.cbSize=sizeof(WNDCLASSEX);
    wc.lpfnWndProc=WindowProc; // 默认窗口过程
    wc.hInstance=impl->getInstance();
    wc.lpszClassName=impl->getClassName();
    wc.hbrBackground=reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1)); // 设置背景颜色

    if(!RegisterClassEx(&wc)) {
        impl->log(LogLevel::Fatal, "Failed to register window class." + std::to_string(GetLastError()));
        MessageBoxEx(nullptr, "Failed to register window class", "Fatal", MB_OK | MB_ICONERROR, 0);
    }
}

void MyWindowX::Unregister() const {
    if(!UnregisterClass(impl->getClassName(), impl->getInstance())) {
        impl->log(LogLevel::Fatal, "Failed to unregister window class.");
        MessageBoxEx(nullptr, "Failed to unregister window class", "Fatal", MB_OK | MB_ICONERROR, 0);
    }
}

LRESULT CALLBACK MyWindowX::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MyWindowX* tObj=nullptr;

    if (uMsg==WM_NCCREATE) {
        const auto cs=reinterpret_cast<CREATESTRUCT*>(lParam);
        tObj=static_cast<MyWindowX*>(cs->lpCreateParams);
        if (tObj==nullptr) return FALSE;
        tObj->setHandle(hwnd);
        ::SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(tObj));
        return TRUE;
    }

    if (uMsg==WM_CLOSE) {
        PostQuitMessage(0);
        return 0;
    }

    tObj=reinterpret_cast<MyWindowX*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (tObj==nullptr) return DefWindowProc(hwnd, uMsg, wParam, lParam);

    tObj->impl->setMessage(uMsg);
    tObj->impl->runJob();
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void MyWindowX::Update() const {
    // 更新窗口内容
    InvalidateRect(impl->getHandle(), nullptr, TRUE);
    UpdateWindow(impl->getHandle());
}

void MyWindowX::messageLoop() const {
    MSG msg;
    BOOL bRet=0;

    while (true) {
        bRet=GetMessageW(&msg,nullptr,0,0);
        if (bRet==0)break; // WM_QUIT
        else if (bRet==-1) {
            impl->log(LogLevel::Error, "GetMessage failed: " + std::to_string(GetLastError()));
            break;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}
