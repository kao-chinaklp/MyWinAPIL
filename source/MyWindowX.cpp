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

        void log(const LogLevel level, const std::string& msg) const {logger->WriteLog(level, msg);}

    private:
        HWND hwnd; // 窗口句柄
        HINSTANCE hInstance; //程序实例句柄
        LPCSTR className; // 窗口类名
        LPCSTR windowName; // 窗口标题

        std::shared_ptr<MyLogger> logger;
};

MyWindowX::MyWindowX(LPCSTR className, LPCSTR windowName, HINSTANCE instance, std::shared_ptr<MyLogger> logger) {
    impl=std::make_unique<MyWindowXImpl>(className, windowName, instance, std::move(logger));

    // 注册窗口类
    Register();
}

MyWindowX::~MyWindowX() {
    // 销毁窗口
    if(impl->getHandle()!=nullptr)Destroy();

    // 注销窗口类
    Unregister();
}

void MyWindowX::Create(LPCSTR className, LPCSTR windowName, HINSTANCE instance,
                       HWND parent, LPVOID data, int width, int height, int x, int y) const {
    auto tmp = CreateWindowEx(
        0, className, windowName, WS_OVERLAPPEDWINDOW,
        x, y, width, height, parent, nullptr, instance, data
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

void MyWindowX::show(int nCmdShow) const {
    if (impl->getHandle()!=nullptr) {
        ShowWindow(impl->getHandle(), nCmdShow);
        UpdateWindow(impl->getHandle());
    }
}

void MyWindowX::Register() const {
    WNDCLASSEX wc={};
    wc.lpfnWndProc=DefWindowProc; // 默认窗口过程
    wc.hInstance=impl->getInstance();
    wc.lpszClassName=impl->getClassName();
    wc.hbrBackground=reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1)); // 设置背景颜色

    if(!RegisterClassEx(&wc)) {
        impl->log(LogLevel::Fatal, "Failed to register window class.");
        MessageBoxEx(nullptr, "Failed to register window class", "Fatal", MB_OK | MB_ICONERROR, 0);
    }
}

void MyWindowX::Unregister() const {
    if(!UnregisterClass(impl->getClassName(), impl->getInstance())) {
        impl->log(LogLevel::Fatal, "Failed to unregister window class.");
        MessageBoxEx(nullptr, "Failed to unregister window class", "Fatal", MB_OK | MB_ICONERROR, 0);
    }
}

LRESULT CALLBACK MyWindowX::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, void (*Function)(void *), void *data) {
    std::shared_ptr<MyWindowX> tObj=nullptr;

    switch(uMsg) {
        case WM_CREATE:
            tObj=std::make_shared<MyWindowX>(this->getClassName(), this->getWindowName(), static_cast<HINSTANCE>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams));
            // 设置窗口的用户数据为当前对象
            tObj->setHandle(hwnd);
            ::SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(tObj.get()));
            break;

        case WM_DESTROY:
            // 处理窗口销毁消息
            PostQuitMessage(0);
            break;

        default:
            Function(data);
            break;
    }

    return 0;
}

void MyWindowX::Update() const {
    // 更新窗口内容
    InvalidateRect(impl->getHandle(), nullptr, TRUE);
    UpdateWindow(impl->getHandle());
}

void MyWindowX::messageLoop() const {
    MSG msg;
    BOOL bRet=0;
    // wchar_t str[256];
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
