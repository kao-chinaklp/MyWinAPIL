#ifndef MYWINDOWX_H
#define MYWINDOWX_H

#include <memory>
#include <windows.h>

#include "MyWindowX.h"

class MyWindowX {
    public:
        MyWindowX(LPCSTR className, LPCSTR windowName, HINSTANCE instance);
        ~MyWindowX();

        void Create(LPCSTR className, LPCSTR windowName, HINSTANCE instance,
                    int width=800, int height=600, int x=CW_USEDEFAULT, int y=CW_USEDEFAULT, HWND parent, LPVOID data) const;
        void Destroy();

    protected:
        [[nodiscard]] HWND getHandle() const;
        [[nodiscard]] HINSTANCE getInstance() const;
        [[nodiscard]] LPCSTR getClassName() const;
        [[nodiscard]] LPCSTR getWindowName() const;

        void setHandle(HWND handle);

        void show(int nCmdShow) const;

    protected:
        void Register() const;
        void Unregister() const;

        int handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) const;

        LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void update() const;

    private:
        class MyWindowXImpl;
        std::unique_ptr<MyWindowXImpl> impl;
};

#endif //MYWINDOWX_H
