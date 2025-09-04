#ifndef MYWINDOWX_H
#define MYWINDOWX_H

#include <memory>
#include <windows.h>

#include "MyLogger.h"
#include "MyWindowX.h"

class MyWindowX {
    public:
        MyWindowX(LPCSTR className, LPCSTR windowName, void (*Job)(UINT), HINSTANCE instance=nullptr, std::shared_ptr<MyLogger> logger=nullptr,
            HWND parent=nullptr, LPVOID data=nullptr, int width=800, int height=600, int x=CW_USEDEFAULT, int y=CW_USEDEFAULT);
        ~MyWindowX();
        void show(int nCmdShow) const;

        void Destroy();

        void ChangeIcon(const std::string& iconName); // Path to ".ico" file.

    protected:
        [[nodiscard]] HWND getHandle() const;
        [[nodiscard]] HINSTANCE getInstance() const;
        [[nodiscard]] LPCSTR getClassName() const;
        [[nodiscard]] LPCSTR getWindowName() const;

        void setHandle(HWND handle);

        void getJob(void (*func)(UINT uMsg));

    protected:
        void Register() const;

        void Create(LPCSTR className, LPCSTR windowName, HINSTANCE instance,
                    HWND parent, LPVOID data, int width=800, int height=600, int x=CW_USEDEFAULT, int y=CW_USEDEFAULT) const;

        void Unregister() const;

        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void Update() const;

        void messageLoop() const;

    private:
        class MyWindowXImpl;
        std::unique_ptr<MyWindowXImpl> impl;
};

#endif //MYWINDOWX_H
